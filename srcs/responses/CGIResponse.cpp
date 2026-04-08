/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/04/08 11:55:59 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIResponse.hpp"
#include "../logger/Logger.hpp"
#include "ErrorResponse.hpp"

CGIResponse::CGIResponse(Location *loc, Request *req, const int &port) :
	Response(loc, req) {
	_headerSent = 0;
	_port = port;
}

CGIResponse::~CGIResponse(void) {}

int CGIResponse::generateHeader(void) {
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	char buf[64];
	getTime(buf, 64);
	header << "Date: " << buf << "\r\n";
	header << "Server: " << SERVER_NAME << "\r\n";
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "Transfer-Encoding: chunked\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

bool CGIResponse::sendResponse(const int &clientFD) {
	try {
		ssize_t ret = 0;
		if (!_headerSent) {
			LOG(Logger::LOG, "CGIResponse: sendResponse");
			setResponseBody();
			generateHeader();
			ret = send(clientFD, _responseHeader.c_str(),
					   _responseHeader.size(), 0);
			if (ret < 0)
				throw std::runtime_error(
					"CGIResponse: sendResponse: send failure");
			write(1, _responseHeader.c_str(), _responseHeader.size());
			_headerSent = 1;
		}

		static ssize_t chunk = CHUNK_SIZE;
		static ssize_t totalSent = 0;
		static ssize_t bodySize = _responseBody.size();
		if (totalSent >= bodySize) {
			ret = send(clientFD, "0\r\n\r\n", 5, 0);
			if (ret < 0)
				throw std::runtime_error(
					"CGIResponse: sendResponse: send failure");
			return DONE;
		}

		int toSend = std::min(chunk, bodySize - totalSent);
		std::string hex = toHex(toSend);
		ret = send(clientFD, hex.c_str(), hex.size(), 0);
		if (ret < 0)
			throw std::runtime_error("CGIResponse: sendResponse: send failure");
		write(1, hex.c_str(), hex.size());
		ret = send(clientFD, _responseBody.c_str() + totalSent, toSend, 0);
		if (ret < 0)
			throw std::runtime_error("CGIResponse: sendResponse: send failure");
		write(1, _responseBody.c_str() + totalSent, toSend);
		ret = send(clientFD, "\r\n", 2, 0);
		if (ret < 0)
			throw std::runtime_error("CGIResponse: sendResponse: send failure");
		write(1, "\r\n", 2);
		totalSent += toSend;
		LOG(Logger::LOG, "CGIResponse: sendResponse");
		return ONGOING;
	} catch (std::exception &e) {
		LOG(Logger::ERROR, e.what());
		ErrorResponse error(_location, NULL);
		error.setErrorCode(500);
		error.sendResponse(clientFD);
	}
	return DONE;
}

static int executeCGI(void) {
	char *fileName = getenv("SCRIPT_NAME");
	char *argv[] = {fileName, NULL};
	execve(fileName, argv, environ);
	return 1;
}

static int executePython(void) {
	char *fileName = getenv("SCRIPT_NAME");
	char py[17] = "/usr/bin/python3";
	char *argv[] = {(char *)&py[0], fileName, NULL};
	execve((char *)&py[0], argv, environ);
	return 1;
}

static int executePHP(void) {
	char *fileName = getenv("SCRIPT_NAME");
	char php[13] = "/usr/bin/php";
	char *argv[] = {(char *)&php[0], fileName, NULL};
	execve((char *)&php[0], argv, environ);
	return 1;
}
int CGIResponse::childProcess(const int (&pipeP2C)[2],
							  const int (&pipeC2P)[2]) {
	_CGIFileType = PHP;
	setEnvironment();
	close(pipeC2P[0]);
	close(pipeP2C[1]);
	if (dup2(pipeC2P[1], STDOUT_FILENO) < 0
		|| dup2(pipeP2C[0], STDIN_FILENO) < 0) {
		perror("dup2");
		exit(1);
	}
	close(pipeC2P[1]);
	close(pipeP2C[0]);
	switch (_CGIFileType) {
	case CGI:
		executeCGI();
		break;
	case PY:
		executePython();
		break;
	case PHP:
		executePHP();
	}
	perror("child execution");
	exit(1);
}

int CGIResponse::setResponseBody(void) {
	int pipeP2C[2];
	int pipeC2P[2];
	if (pipe(pipeP2C) == -1 || pipe(pipeC2P) == -1)
		throw std::runtime_error("setResponseBody: pipe failure");
	int pid = fork();
	if (pid == -1)
		throw std::runtime_error("setResponseBody: fork failure");
	if (pid == 0)
		childProcess(pipeP2C, pipeC2P);
	close(pipeP2C[0]);
	close(pipeC2P[1]);
	const std::string &body = _request->getBody();
	write(pipeP2C[1], body.c_str(), body.size());
	close(pipeP2C[1]);
	ssize_t res = 1;
	ssize_t chunk = 8192;
	ssize_t size;
	_responseBody.resize(chunk);
	while (res > 0) {
		size = _responseBody.size();
		res = read(pipeC2P[0], &_responseBody[size - chunk], chunk);
		if (res == -1)
			throw std::runtime_error("setResponseBody: read failure");
		if (res < chunk)
			break;
		_responseBody.resize(size + chunk);
	}
	if (res != -1) {
		size = strlen(_responseBody.c_str());
		_responseBody.resize(strlen(_responseBody.c_str()));
	}
	close(pipeC2P[0]);
	int status = 0;
	waitpid(pid, &status, WNOHANG);
	if (WIFEXITED(status))
		status = WEXITSTATUS(status);
	if (status != 0)
		throw std::runtime_error("child process: execution failure");
	printf("[written: %lu] [status: %d]\n\n", _responseBody.size(), status);
	return (0);
}

std::string *CGIResponse::getCGIoutput(void) { return (&_responseBody); }

void CGIResponse::setEnvironment(void) {
	// if (_metaVs.AUTH_TYPE)
	// 	setenv("AUTH_TYPE", _metaVs.AUTH_TYPE, 1);
	// else
	// 	setenv("AUTH_TYPE", "NULL", 1);

	const int &contentLength = _request->getContentLength();
	if (contentLength >= 0) {
		char buf[32];
		::snprintf(buf, 32, "%d", contentLength);
		setenv("CONTENT_LENGTH", buf, 1);
	}
	// else
	// 	setenv("CONTENT_LENGTH", "", 1);

	const std::string &contentType = _request->getContentType();
	if (!contentType.empty())
		setenv("CONTENT_TYPE", contentType.c_str(), 1);
	// else
	// 	setenv("CONTENT_TYPE", "", 1);

	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);

	const std::string &requestPath = _request->getRequestPath();
	if (!requestPath.empty())
		setenv("PATH_INFO", requestPath.c_str(), 1);
	else
		setenv("PATH_INFO", "", 1);

	// if (_metaVs.PATH_TRANSLATED)
	// setenv("PATH_TRANSLATED", _metaVs.PATH_TRANSLATED, 1);
	// else
	// 	setenv("PATH_TRANSLATED", "NULL", 1);

	const std::string &queryString = _request->getQueryString();
	if (!queryString.empty())
		setenv("QUERY_STRING", queryString.c_str(), 1);
	// else
	// 	setenv("QUERY_STRING", "NULL", 1);

	const std::string &remoteAddr = _request->getRemoteAddr();
	if (!remoteAddr.empty())
		setenv("REMOTE_ADDR", remoteAddr.c_str(), 1);
	// else
	// 	setenv("REMOTE_ADDR", "NULL", 1);

	const std::string &remoteHost = _request->getRemoteHost();
	if (!remoteHost.empty())
		setenv("REMOTE_HOST", remoteHost.c_str(), 1);
	// else
	// 	setenv("REMOTE_HOST", "NULL", 1);

	// if (_metaVs.REMOTE_IDENT)
	// 	setenv("REMOTE_IDENT", _metaVs.REMOTE_IDENT, 1);
	// else
	// 	setenv("REMOTE_IDENT", "NULL", 1);

	// if (_metaVs.REMOTE_USER)
	// 	setenv("REMOTE_USER", _metaVs.REMOTE_USER, 1);
	// else
	// 	setenv("REMOTE_USER", "NULL", 1);

	switch (_request->getType()) {
	case REQ_GET:
		setenv("REQUEST_METHOD", "GET", 1);
		break;
	case REQ_POST:
		setenv("REQUEST_METHOD", "POST", 1);
		break;
	case REQ_DELETE:
		setenv("REQUEST_METHOD", "DELETE", 1);
		break;
	case REQ_ERROR:
		break;
	}
	// else
	// 	setenv("REQUEST_METHOD", "NULL", 1);

	const std::string &scriptName = _request->getFilePath();
	std::cout << "script name: " << scriptName << std::endl;
	if (!scriptName.empty())
		setenv("SCRIPT_NAME", scriptName.c_str(), 1);
	// else
	// 	setenv("SCRIPT_NAME", "NULL", 1);

	// if (_metaVs.SERVER_NAME)
	setenv("SERVER_NAME", "PNC webserv", 1);
	// else
	// 	setenv("SERVER_NAME", "NULL", 1);

	// if (_metaVs.SERVER_PORT)
	char buf[32];
	// ::snprintf(buf, 32, "%d", _requestVars->port);
	setenv("SERVER_PORT", buf, 1);
	// else
	// 	setenv("SERVER_PORT", "NULL", 1);

	// if (_metaVs.SERVER_PROTOCOL)
	setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
	// else
	// 	setenv("SERVER_PROTOCOL", "NULL", 1);

	// if (_metaVs.SERVER_SOFTWARE)
	setenv("SERVER_SOFTWARE", SERVER_NAME, 1);
	// else
	// 	setenv("SERVER_SOFTWARE", "NULL", 1);
}

std::string toHex(int val) {
	std::ostringstream oss;
	oss << std::hex << val << "\r\n";
	return oss.str();
}

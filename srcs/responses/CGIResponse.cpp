/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/04/10 17:27:36 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIResponse.hpp"
#include "ErrorResponse.hpp"
#include "../logger/Logger.hpp"
#include <cstring>
#include <cstdlib>



CGIResponse::CGIResponse(Location* loc, Request* req, const int& port) :
	Response(loc, req),
	_headerSent(0),
	_totalRead(0),
	_port(port),
	_readState(FULL)
{
	_requestBody.reserve(RECV_SIZE + 1);
}

CGIResponse::~CGIResponse(void) {}

int	CGIResponse::generateHeader(void)
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: " << SERVER_NAME << "\r\n";
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "Transfer-Encoding: chunked\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

void	CGIResponse::initChild(void)
{
	if (pipe(_pipeP2C) == -1 || pipe(_pipeC2P) == -1)
		throw std::runtime_error("CGIResponse: initChild: pipe failure");
	int pid = fork();
	if (pid == -1)
		throw std::runtime_error("CGIResponse: initChild: fork failure");
	if (pid == 0)
		childProcess();
	close(_pipeP2C[0]);
	close(_pipeC2P[1]);
}

static ssize_t	strCopyCount(char *dest, const char *src)
{
	ssize_t i;

	for (i = 0; src[i] != '\0'; i++)
		dest[i] = src[i];

	return i;
}

bool	CGIResponse::readBodyChunked(char buf[], ssize_t bytesRead, ssize_t& start)
{
	ssize_t	ret = 0;
	if (_readBufEnd >= bytesRead)
		throw std::invalid_argument("CGIResponse: rBC: invalid chunked body");
	ret = write(_pipeP2C[1], _readBuffer, _readBufEnd);
	if (ret < _readBufEnd)
		throw std::runtime_error("CGIResponse: rBC: write failure");
	start = _toRead - _readBufEnd;
	ret = write(_pipeP2C[1], buf, start);
	if (ret < _readBufEnd)
		throw std::runtime_error("CGIResponse: rBC: write failure");
	return ONGOING;
}

char*	CGIResponse::readLenChunked(char buf[], ssize_t bytesRead, ssize_t& start)
{
	ssize_t	ret = 0;
	char*	nextRN = strstr(buf, "/r/n");
	if (nextRN == NULL)
			throw std::invalid_argument("CGIResponse: rLC: invalid chunked body");
	start = nextRN - buf;
	strncpy(&_readBuffer[_readBufEnd], buf, start);
	_readBufEnd += start;
	char* check;
	_toRead = strtol(_readBuffer, &check, 16);
	if (check == NULL || check == _readBuffer)
			throw std::invalid_argument("CGIResponse: rLC: invalid chunked body");
	return nextRN;
}

bool	CGIResponse::readBodyFull(char buf[], ssize_t bytesRead)
{
	ssize_t writeAmt = std::min(_request->getContentLength(), static_cast<size_t>(bytesRead));
	ssize_t ret = write(_pipeP2C[1], &buf, writeAmt);
	if (ret < writeAmt)
		throw std::runtime_error("CGIResponse: readBodyFirst: write failure");
	_totalRead += ret;
	if (_totalRead >= _request->getContentLength())
	{
		close(_pipeP2C[1]);
		return DONE;
	}
	return ONGOING;
}

bool	CGIResponse::readBodyLoop(char buf[], ssize_t bytesRead)
{
	ssize_t	start = 0;
	ssize_t	toRead = 0;
	ssize_t	ret = 0;
	char*	nextRN = NULL;
	switch (_readState) {
		case CHUNK_LEN :
			nextRN = readLenChunked(buf, bytesRead, start);
			goto bod;
		case CHUNK_BOD :
			readBodyChunked(buf, bytesRead, start);
			break ;
		case FULL :
			return readBodyFull(buf, bytesRead);
	}

	while (start < bytesRead)
	{
		toRead = strtol(&buf[start], &nextRN, 16);
		if (nextRN == NULL || nextRN == &buf[start])
			throw std::invalid_argument("CGIResponse: rBFC: invalid chunked body");
		start = nextRN + 2 - buf;
		if (nextRN >= &buf[bytesRead])
		{
			_readBufEnd = strCopyCount(_readBuffer, &buf[start]);
			_readState = CHUNK_LEN;
			return ONGOING;
		}
bod:
		if (toRead == 0)
			return DONE;
		if (start + toRead > bytesRead)
		{
			if (start > bytesRead)
				_readBufEnd = 0;
			else
				_readBufEnd = strCopyCount(_readBuffer, &buf[start]);
			_readState = CHUNK_BOD;
			return ONGOING;
		}
		ret = write(_pipeP2C[1], &buf[start], toRead);
		if (ret < toRead)
			throw std::runtime_error("CGIResponse: readBodyFirst: write failure");
		start += toRead;

	}
}



bool	CGIResponse::readBodyFirstChunked(char buf[], ssize_t bytesRead)
{
	char* nextRN = NULL;
	ssize_t start = _request->getHeaderEnd();
	ssize_t	ret;
	if (start + 4 > bytesRead)
	{
		_readBufEnd = 0;
		return ONGOING;
	}
	while (start < bytesRead)
	{
		_toRead = strtol(&buf[start], &nextRN, 16);
		if (nextRN == NULL || nextRN == &buf[start])
			throw std::invalid_argument("CGIResponse: rBFC: invalid chunked body");
		start = nextRN + 2 - buf;
		if (nextRN >= &buf[bytesRead])
		{
			_readBufEnd = strCopyCount(_readBuffer, &buf[start]);
			_readState = CHUNK_LEN;
			return ONGOING;
		}
		if (_toRead == 0)
			return DONE;
		if (start + _toRead > bytesRead)
		{
			if (start > bytesRead)
				_readBufEnd = 0;
			else
				_readBufEnd = strCopyCount(_readBuffer, &buf[start]);
			_readState = CHUNK_BOD;
			return ONGOING;
		}
		ret = write(_pipeP2C[1], &buf[start], _toRead);
		if (ret < _toRead)
			throw std::runtime_error("CGIResponse: rBFC: write failure");
		start += _toRead;
	}
	_readBufEnd = 0;
	_readState = CHUNK_LEN;
	return ONGOING;
}

bool	CGIResponse::readBodyFirst(char buf[], ssize_t bytesRead)
{
	initChild();
	if (_request->getTransferEncoding() == "chunked")
		return readBodyFirstChunked(buf, bytesRead);
	ssize_t start = _request->getHeaderEnd() + 4;
	if (start > bytesRead)
		return ONGOING;
	ssize_t writeAmt = std::min(_request->getContentLength(), static_cast<size_t>(bytesRead - start));
	ssize_t ret = write(_pipeP2C[1], &buf[start], writeAmt);
	if (ret < writeAmt)
		throw std::runtime_error("CGIResponse: readBodyFirst: write failure");
	_totalRead += ret;
	if (_totalRead >= _request->getContentLength())
	{
		close(_pipeP2C[1]);
		return DONE;
	}
	return ONGOING;
}
/*
int		CGIResponse::setResponseBody(void)
{

	const std::string& body = _request->getBody();
	write(pipeP2C[1], body.c_str(), body.size());
	close(pipeP2C[1]);
	ssize_t	res = 1;
	ssize_t	chunk = 8192;
	ssize_t	size;
	ssize_t	oldSize = 0;
	_responseBody.resize(chunk);
	while (res > 0)
	{
		size = _responseBody.size();
		res = read(pipeC2P[0], &_responseBody[size - chunk], chunk);
		if (res == -1)
			throw std::runtime_error("setResponseBody: read failure");
		if (res < chunk)
			break ;
		oldSize = size;
		_responseBody.resize(size + chunk);
	}
	if (res != -1)
	{
		size = strlen(_responseBody.c_str());
		_responseBody.resize(strlen(_responseBody.c_str()));
	}
	close(pipeC2P[0]);
	int	status = 0;
	waitpid(pid, &status, WNOHANG);
	if (WIFEXITED(status))
		status = WEXITSTATUS(status);
	if (status != 0)
		throw std::runtime_error("child process: execution failure");
	printf("[written: %lu] [status: %d]\n\n", _responseBody.size(), status);
	return (0);
} */

bool	CGIResponse::sendResponse(const int &clientFD)
{
	try {
		ssize_t	ret = 0;
		if (!_headerSent)
		{
			LOG(Logger::LOG, "CGIResponse: sending response");
			// setResponseBody();
			generateHeader();
			ret = send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
			if (ret < 0)
				throw std::runtime_error("CGIResponse: sendResponse: send failure");
			write(1, _responseHeader.c_str(), _responseHeader.size());
			_headerSent = 1;
		}

		static ssize_t	chunk = CHUNK_SIZE;
		static ssize_t	totalSent = 0;
		static ssize_t	bodySize = _responseBody.size();
		if (totalSent >= bodySize)
		{
			ret = send(clientFD, "0\r\n\r\n", 5, 0);
			LOG(Logger::LOG, "CGIResponse: response sent");
			if (ret < 0)
				throw std::runtime_error("CGIResponse: sendResponse: send failure");
			return DONE;
		}

		int	toSend = std::min(chunk, bodySize - totalSent);
		std::string	hex = toHex(toSend);
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
		LOG(Logger::CONTENT, "CGIResponse: Sent to client:");
		LOG(Logger::CONTENT, _responseHeader.c_str());
		LOG(Logger::CONTENT, _responseBody.c_str());
		return ONGOING;
	}
	catch (std::exception &e) {
		LOG(Logger::ERROR, e.what());
		ErrorResponse error(_location, NULL);
		error.setErrorCode(500);
		error.sendResponse(clientFD);
	}
	return DONE;
}

static int	executeCGI(void)
{
	char*	fileName = getenv("SCRIPT_NAME");
	char*	argv[] = { fileName, NULL };
	execve(fileName, argv, environ);
	return 1;
}

static int	executePython(void)
{
	char*	fileName = getenv("SCRIPT_NAME");
	char	py[17] = "/usr/bin/python3";
	char*	argv[] = { (char*)&py[0], fileName, NULL };
	execve ((char*)&py[0], argv, environ);
	return 1;
}

static int	executePHP(void)
{
	char*	fileName = getenv("SCRIPT_NAME");
	char	php[13] = "/usr/bin/php";
	char*	argv[] = { (char*)&php[0], fileName, NULL };
	execve ((char*)&php[0], argv, environ);
	return 1;
}
int	CGIResponse::childProcess(void)
{
	_CGIFileType = PHP;
	setEnvironment();
	close(_pipeC2P[0]);
	close(_pipeP2C[1]);
	if (dup2(_pipeC2P[1], STDOUT_FILENO) < 0 || dup2(_pipeP2C[0], STDIN_FILENO) < 0)
	{
		LOG(Logger::ERROR, "CGIResponse: child execution dup2 error");
		exit(1);
	}
	close(_pipeC2P[1]);
	close(_pipeP2C[0]);
	LOG(Logger::LOG, "CGIResponse: child process executing");
	switch (_CGIFileType) {
		case CGI:
			executeCGI();
			break ;
		case PY:
			executePython();
			break ;
		case PHP:
			executePHP();
	}
	LOG(Logger::ERROR, "CGIResponse: child execution error");
	exit (1);
}

std::string	*CGIResponse::getCGIoutput(void)
{
	return (&_responseBody);
}

void	CGIResponse::setEnvironment(void)
{
	const int& contentLength = _request->getContentLength();
	if (contentLength >= 0)
	{
		char	buf[32];
		::snprintf(buf, 32, "%d", contentLength);
		setenv("CONTENT_LENGTH", buf, 1);
	}

	const std::string& contentType = _request->getContentType();
	if (!contentType.empty())
		setenv("CONTENT_TYPE", contentType.c_str(), 1);

	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);

	const std::string& requestPath = _request->getRequestPath();
	if (!requestPath.empty())
		setenv("PATH_INFO", requestPath.c_str(), 1);
	else
		setenv("PATH_INFO", "", 1);

	const std::string& queryString = _request->getQueryString();
	if (!queryString.empty())
		setenv("QUERY_STRING", queryString.c_str(), 1);

	const std::string& remoteAddr = _request->getRemoteAddr();
	if (!remoteAddr.empty())
		setenv("REMOTE_ADDR", remoteAddr.c_str(), 1);

	const std::string& remoteHost = _request->getRemoteHost();
	if (!remoteHost.empty())
		setenv("REMOTE_HOST", remoteHost.c_str(), 1);

	switch (_request->getType()) {
		case REQ_GET:
			setenv("REQUEST_METHOD", "GET", 1);
			break ;
		case REQ_POST:
			setenv("REQUEST_METHOD", "POST", 1);
			break ;
		case REQ_DELETE:
			setenv("REQUEST_METHOD", "DELETE", 1);
			break ;
		case REQ_ERROR:
			break ;
	}

	const std::string& scriptName = _request->getFilePath();
	std::cout << "script name: " << scriptName << std::endl;
	if (!scriptName.empty())
		setenv("SCRIPT_NAME", scriptName.c_str(), 1);

	setenv("SERVER_NAME", "PNC webserv", 1);

	char	buf[32];
	// ::snprintf(buf, 32, "%d", _requestVars->port);
	setenv("SERVER_PORT", buf, 1);

	setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);

	setenv("SERVER_SOFTWARE", SERVER_NAME, 1);
}

std::string	strToHex(int val)
{
	std::ostringstream	oss;
	oss << std::hex << val << "\r\n";
	return oss.str();
}

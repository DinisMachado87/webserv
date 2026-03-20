/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/03/19 18:23:06 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIResponse.hpp"



CGIResponse::CGIResponse(Location* loc, reqVariables* vars) : Response(loc, vars)
{

}
CGIResponse::~CGIResponse(void)
{

}

int	CGIResponse::createHeader(void)
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: nailedIt/1.0\r\n";
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

int	CGIResponse::sendResponse(const int &clientFD)
{
	if (runCGI() != 0)
		return (1);
	createHeader();
	send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
	send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
	std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	return 0;
}

int	CGIResponse::childProcess(int pipeP2C[2], int pipeC2P[2])
{
	this->setEnvironment();
	close(pipeC2P[0]);
	close(pipeP2C[1]);
	if (dup2(pipeC2P[1], STDOUT_FILENO) < 0 || dup2(pipeP2C[0], STDIN_FILENO) < 0)
	{
		perror("dup2");
		exit(1);
	}
	char*	fileName = getenv("SCRIPT_NAME");
	char* argv[] = { fileName, NULL };
	close(pipeC2P[1]);
	close(pipeP2C[0]);
	execve (fileName, argv, environ);
	(void)argv;
	perror("child execution");
	// write(1, "lalala", 6);
	exit (1);
}

int		CGIResponse::runCGI(void)
{
	int	pipeP2C[2];
	int	pipeC2P[2];
	if (pipe(pipeP2C) == -1 || pipe(pipeC2P))
	{
		perror("pipe");
		return (-1);
	}
	int pid = fork();
	if (pid == -1)
	{
		perror("fork");
		return(-1);
	}
	if (pid == 0)
		this->childProcess(pipeP2C, pipeC2P);
	close(pipeP2C[0]);
	close(pipeC2P[1]);
	write(pipeP2C[1], this->_requestVars->body.c_str(), this->_requestVars->body.size());
	close(pipeP2C[1]);
	int	status = 0;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		status = WEXITSTATUS(status);
	ssize_t	res = 1;
	ssize_t	chunk = 8132;
	ssize_t	size;
	ssize_t	oldSize = 0;
	this->_responseBody.resize(chunk);
	while (res > 0)
	{
		size = this->_responseBody.size();
		res = read(pipeC2P[0], &this->_responseBody[size - chunk], chunk);
		if (res < chunk)
			break ;
		oldSize = size;
		this->_responseBody.resize(size + chunk);
	}
	if (res != -1)
		this->_responseBody.resize(oldSize + res);
	printf("[written: %lu] [status: %d]\n\n", this->_responseBody.size(), status);
	close(pipeC2P[0]);
	return (0);
}

std::string	*CGIResponse::getCGIoutput(void)
{
	return (&this->_responseBody);
}

void	CGIResponse::setEnvironment(void)
{
	if (this->_metaVs.AUTH_TYPE)
		setenv("AUTH_TYPE", this->_metaVs.AUTH_TYPE, 1);
	// else
	// 	setenv("AUTH_TYPE", "NULL", 1);

	if (this->_requestVars->contentLength >= 0)
	{
		char	buf[32];
		::snprintf(buf, 32, "%ld", this->_requestVars->contentLength);
		setenv("CONTENT_LENGTH", buf, 1);
	}
	// else
	// 	setenv("CONTENT_LENGTH", "", 1);

	if (!this->_requestVars->contentType.empty())
		setenv("CONTENT_TYPE", this->_requestVars->contentType.c_str(), 1);
	// else
	// 	setenv("CONTENT_TYPE", "", 1);

	if (this->_metaVs.GATEWAY_INTERFACE)
		setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);


	if (!this->_requestVars->requestPath.empty())
		setenv("PATH_INFO", this->_requestVars->requestPath.c_str(), 1);
	else
		setenv("PATH_INFO", "", 1);

	// if (this->_metaVs.PATH_TRANSLATED)
		// setenv("PATH_TRANSLATED", this->_metaVs.PATH_TRANSLATED, 1);
	// else
	// 	setenv("PATH_TRANSLATED", "NULL", 1);

	if (!this->_requestVars->queryString.empty())
		setenv("QUERY_STRING", this->_requestVars->queryString.c_str(), 1);
	// else
	// 	setenv("QUERY_STRING", "NULL", 1);

	if (!this->_requestVars->remoteAddr.empty())
		setenv("REMOTE_ADDR", this->_requestVars->remoteAddr.c_str(), 1);
	// else
	// 	setenv("REMOTE_ADDR", "NULL", 1);

	if (!this->_requestVars->remoteHost.empty())
		setenv("REMOTE_HOST", this->_requestVars->remoteHost.c_str(), 1);
	// else
	// 	setenv("REMOTE_HOST", "NULL", 1);

	// if (this->_metaVs.REMOTE_IDENT)
	// 	setenv("REMOTE_IDENT", this->_metaVs.REMOTE_IDENT, 1);
	// else
	// 	setenv("REMOTE_IDENT", "NULL", 1);

	// if (this->_metaVs.REMOTE_USER)
	// 	setenv("REMOTE_USER", this->_metaVs.REMOTE_USER, 1);
	// else
	// 	setenv("REMOTE_USER", "NULL", 1);

	switch (this->_requestVars->type) {
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
	// else
	// 	setenv("REQUEST_METHOD", "NULL", 1);

	if (!this->_requestVars->scriptName.empty())
		setenv("SCRIPT_NAME", this->_requestVars->scriptName.c_str(), 1);
	// else
	// 	setenv("SCRIPT_NAME", "NULL", 1);

	// if (this->_metaVs.SERVER_NAME)
	setenv("SERVER_NAME", "PNC webserv", 1);
	// else
	// 	setenv("SERVER_NAME", "NULL", 1);

	// if (this->_metaVs.SERVER_PORT)
	// char	buf[32];
	// ::snprintf(buf, 32, "%d", this->_requestVars->port);
	// setenv("SERVER_PORT", buf, 1);
	// else
	// 	setenv("SERVER_PORT", "NULL", 1);

	// if (this->_metaVs.SERVER_PROTOCOL)
	setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
	// else
	// 	setenv("SERVER_PROTOCOL", "NULL", 1);

	// if (this->_metaVs.SERVER_SOFTWARE)
		setenv("SERVER_SOFTWARE", "PNC server 1.0", 1);
	// else
	// 	setenv("SERVER_SOFTWARE", "NULL", 1);
}

void	CGIResponse::initialiseMetaVs(void)
{
	this->_metaVs.AUTH_TYPE = strdup("user");	//temp
	// const char*	lenStr = std::to_string(this->_requestVars->contentLength).c_str();
	// this->_metaVs.CONTENT_LENGTH = strdup(lenStr);
	this->_metaVs.CONTENT_TYPE = strdup("text");
	this->_metaVs.GATEWAY_INTERFACE = strdup("CGI/1.1");
	this->_metaVs.PATH_INFO = NULL;
	this->_metaVs.PATH_TRANSLATED = NULL;
	this->_metaVs.QUERY_STRING = strdup("query=hi?");
	this->_metaVs.REMOTE_ADDR = strdup("127.0.0.21");
	this->_metaVs.REMOTE_HOST = strdup("NULL");
	this->_metaVs.REMOTE_IDENT = strdup("NULL");
	this->_metaVs.REMOTE_USER = strdup("NULL");
	this->_metaVs.REQUEST_METHOD = strdup("GET");
	this->_metaVs.SCRIPT_NAME = strdup("includes/cgi-bin/hello.cgi");
	this->_metaVs.SERVER_NAME = strdup("webserv");
	this->_metaVs.SERVER_PORT = strdup("5555");
	this->_metaVs.SERVER_PROTOCOL = strdup("HTTP/1.1");
	this->_metaVs.SERVER_SOFTWARE = strdup("PNC");
}


void	CGIResponse::freeMetaVs(void)
{
	if (this->_metaVs.AUTH_TYPE)
		free(this->_metaVs.AUTH_TYPE);
	if (this->_metaVs.CONTENT_LENGTH)
		free(this->_metaVs.CONTENT_LENGTH);
	if (this->_metaVs.CONTENT_TYPE)
		free(this->_metaVs.CONTENT_TYPE);
	if (this->_metaVs.GATEWAY_INTERFACE)
		free(this->_metaVs.GATEWAY_INTERFACE);
	if (this->_metaVs.PATH_INFO)
		free(this->_metaVs.PATH_INFO);
	if (this->_metaVs.PATH_TRANSLATED)
		free(this->_metaVs.PATH_TRANSLATED);
	if (this->_metaVs.QUERY_STRING)
		free(this->_metaVs.QUERY_STRING);
	if (this->_metaVs.REMOTE_ADDR)
		free(this->_metaVs.REMOTE_ADDR);
	if (this->_metaVs.REMOTE_HOST)
		free(this->_metaVs.REMOTE_HOST);
	if (this->_metaVs.REMOTE_IDENT)
		free(this->_metaVs.REMOTE_IDENT);
	if (this->_metaVs.REMOTE_USER)
		free(this->_metaVs.REMOTE_USER);
	if (this->_metaVs.REQUEST_METHOD)
		free(this->_metaVs.REQUEST_METHOD );
	if (this->_metaVs.SCRIPT_NAME)
		free(this->_metaVs.SCRIPT_NAME);
	if (this->_metaVs.SERVER_NAME)
		free(this->_metaVs.SERVER_NAME);
	if (this->_metaVs.SERVER_PORT)
		free(this->_metaVs.SERVER_PORT);
	if (this->_metaVs.SERVER_PROTOCOL)
		free(this->_metaVs.SERVER_PROTOCOL);
	if (this->_metaVs.SERVER_SOFTWARE)
		free(this->_metaVs.SERVER_SOFTWARE);
}


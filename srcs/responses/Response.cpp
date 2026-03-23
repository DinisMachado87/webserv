/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/10 14:22:22 by smoon             #+#    #+#             */
/*   Updated: 2026/03/23 14:36:16 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

Response::Response(Location* loc, reqVariables* vars) : _location(loc), _requestVars(vars)
{

}

Response::~Response(void)
{

}

int	Response::generateHeader(void)
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

bool	Response::sendResponse(const int &clientFD, const int &port)
{
	(void) port;			//only needed for CGI
	generateHeader();
	send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
	send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
	std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	return 1;
}

void	Response::getTime(char* buf, int bufSize)
{
	std::time_t current = std::time(NULL);
	std::strftime(buf, bufSize, "%a, %d %b %Y %H:%M:%S", std::localtime(&current));
}

void	initialise_everything(Location* loc, reqVariables* vars, Overrides* over)
{
	(void)loc;
	(void)vars;
	(void)over;
	vars->method = REQ_POST;
	vars->body = "<body> here is some body </body>";
	vars->contentLength = vars->body.size();
	// vars->port = 5555;
	vars->requestPath = "hello.cgi";
	// vars->contentType =
	vars->requestPath = "teams/users";
	vars->queryString = "query=hi";
	vars->scriptName = "hello.cgi";
	vars->remoteAddr = "175.0.0.23";
	vars->remoteHost = "client.com";
}

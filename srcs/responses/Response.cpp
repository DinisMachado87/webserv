/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/10 14:22:22 by smoon             #+#    #+#             */
/*   Updated: 2026/04/08 11:50:20 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "ErrorResponse.hpp"
#include "../logger/Logger.hpp"

Response::Response(Location* loc, Request* req) : _location(loc), _request(req)
{

}

Response::~Response(void)
{
	if (_request)
		delete _request;
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

bool	Response::sendResponse(const int &clientFD)
{
	try {
		setResponseBody();
		generateHeader();
		ssize_t	ret = 0;
		ret = send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		ret = send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		// std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		ErrorResponse error(_location, NULL);
		error.setErrorCode(500);
		error.sendResponse(clientFD);
	}
	return DONE;
}

void	Response::getTime(char* buf, int bufSize)
{
	std::time_t current = std::time(NULL);
	std::strftime(buf, bufSize, "%a, %d %b %Y %H:%M:%S", std::localtime(&current));
}

void	initialise_everything(Location* loc, Request* req, Overrides* over)
{
	(void)loc;
	(void)req;
	(void)over;
	req->setParseError(403, "lalaerrorla");
	req->setType(REQ_POST);
	req->setBody("<body> here is some body </body>");
	req->setContentLength(req->getBody().size());
	// vars->port = 5555;
	req->setFilePath("hello.php");
	// vars->contentType =
	req->setRequestPath("teams/users");
	req->setQueryString("query=hi");
	// vars->scriptName = "hello.cgi";
	req->setRemoteAddr("175.0.0.23");
	req->setRemoteHost("client.com");
}

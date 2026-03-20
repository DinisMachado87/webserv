/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetResponse.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/03/19 17:55:34 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "GetResponse.hpp"



GetResponse::GetResponse(Location* loc, reqVariables* vars) : Response(loc, vars)
{

}
GetResponse::~GetResponse(void)
{

}

int	GetResponse::createHeader(void)
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: nailedIt/1.0\r\n";
	// Content-Type: text/html; charset=UTF-8\r\n
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

int	GetResponse::getResponseBody(void)
{
	errno = 0;
	if (open(this->_requestVars->requestPath.c_str(), O_RDONLY) < 0)
	{
		switch (errno) {
			case EACCES:
				_responseStatus = 403;
				return (403);
		}
	}
	return 0;
}

int	GetResponse::sendResponse(const int &clientFD)
{
	getResponseBody();
	createHeader();
	send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
	send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
	std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	return 0;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetResponse.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/04/07 16:42:39 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "GetResponse.hpp"
#include "ErrorResponse.hpp"

GetResponse::GetResponse(Location* loc, Request* req) : Response(loc, req)
{

}
GetResponse::~GetResponse(void)
{

}

int	GetResponse::generateHeader(void)
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: " << SERVER_NAME << "\r\n";
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

int	GetResponse::setResponseBody(void)
{
	ssize_t	res = 1;
	int		fd = open(_request->getFilePath().c_str(), O_RDONLY);
	if (res < 0)
			throw std::runtime_error("setResponseBody: open failure");
	res = 1;
	ssize_t	chunk = 8192;
	ssize_t	size;
	ssize_t	oldSize = 0;
	_responseBody.resize(chunk);
	while (res > 0)
	{
		size = _responseBody.size();
		res = read(fd, &_responseBody[size - chunk], chunk);
		if (res == -1)
			throw std::runtime_error("setResponseBody: read failure");
		if (res < chunk)
			break ;
		oldSize = size;
		_responseBody.resize(size + chunk);
	}
	if (res != -1)
		_responseBody.resize(oldSize + res);
	return (0);
}



/* bool	GetResponse::sendResponse(const int &clientFD)
{
	setResponseBody();
	generateHeader();
	send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
	send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
	std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	return 1;
} */

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DeleteResponse.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/31 14:25:57 by smoon             #+#    #+#             */
/*   Updated: 2026/04/07 13:26:56 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DeleteResponse.hpp"
#include "ErrorResponse.hpp"



DeleteResponse::DeleteResponse(Location* loc, Request* req) : Response(loc, req)
{

}
DeleteResponse::~DeleteResponse(void)
{

}

int	DeleteResponse::generateHeader(void)
{
	std::stringstream header;
	header << "HTTP/1.1 204 No Content\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: " << SERVER_NAME << "\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

int	DeleteResponse::deleteFile(void)
{
	return (std::remove(this->_request->getFilePath().c_str()));
}

bool	DeleteResponse::sendResponse(const int &clientFD)
{
	try {
		deleteFile();
		generateHeader();
		ssize_t	ret = 0;
		ret = send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		// ret = send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
		// if (ret < 0)
		// 	throw std::runtime_error("sendResponse: send failure");
		std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		ErrorResponse error(_location, NULL);
		error.setErrorCode(500);
		error.sendResponse(clientFD);
	}
	return 0;
}

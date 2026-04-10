/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PostResponse.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/03/26 13:17:10 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "PostResponse.hpp"
#include "ErrorResponse.hpp"
#include "../logger/Logger.hpp"
#include <sys/stat.h>



PostResponse::PostResponse(Location* loc, Request* req) : Response(loc, req)
{

}
PostResponse::~PostResponse(void)
{

}

int	PostResponse::generateHeader(void)
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

static bool	isDir(const char* path)
{
	struct stat info;
	if (stat(path, &info) != 0)
		return false;
	return S_ISDIR(info.st_mode);
}

static std::string	createGenericName(void)
{
	char	name[14];
	char	c;
	int		pos;

	c = '0';
	pos = 7;
	std::memcpy(name, "upload\0\0\0\0\0\0\0", 14);
	while (access(name, F_OK) == 0)
	{
		c++;
		name[pos] = c;
		if (c == '}')
		{
			pos++;
			if (pos == 14)
				break ;
			c = '0';
		}
	}
	return (std::string(name));
}

int	PostResponse::actionPost(void)
{
	std::string	nameStr;
	const char* name;
	if (isDir(name))
	{
		nameStr = createGenericName();
		name = nameStr.c_str();
	}
	else
		name = _request->getFilePath().c_str();
	std::ofstream oss;
	oss.open(name);
	if (!oss)
		throw std::runtime_error("PostResponse: failed to create filestream");
	oss << _request->getBody();
	oss.close();
	LOG(Logger::LOG, "PostResponse: file uploaded");
	return (0);
}

bool	PostResponse::sendResponse(const int &clientFD)
{
	try {
		actionPost();
		generateHeader();
		send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
		send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
		LOG(Logger::LOG, "PostResponse: response sent");
		LOG(Logger::CONTENT, "PostResponse: Sent to client:");
		LOG(Logger::CONTENT, _responseHeader.c_str());
		LOG(Logger::CONTENT, _responseBody.c_str());
	}
	catch (std::exception &e) {
		LOG(Logger::ERROR, e.what());
		ErrorResponse error(_location, NULL);
		error.setErrorCode(500);
		error.sendResponse(clientFD);
	}
	return DONE;
}

int	PostResponse::setResponseBody(void)
{
	std::cout << "PostResponse: No body to set\n";
	return 0;
}

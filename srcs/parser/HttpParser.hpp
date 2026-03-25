/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aviv <aviv@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/20 08:41:13 by akosloff          #+#    #+#             */
/*   Updated: 2026/03/24 15:24:01 by aviv             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <cstddef>
#include <string>
#include "../requests/Request.hpp"

class Server;

class HttpParser
{
public:
	HttpParser();
	~HttpParser();

	Request* parse(char *rawBuffer, size_t bytesRead, int clientFD);

private:
	HttpParser(const HttpParser& other);
	HttpParser& operator=(const HttpParser& other);

private:
	std::string _buffer;
	std::string _fullMessage;

private:
	Request* makeErrorRequest(int clientFD, int code, const std::string& message);
	Request* eraseHeaderAndReturn(Request* err, size_t headerEnd);
	Request* clearBufferAndReturn(Request* err);

	bool firstLineParse(const std::string& firstLine, Request& req);
	bool headerParse(const std::string& headerLine, Request& req, bool& clearBufferOnError);
	bool parseRequestTarget(const std::string& target, Request& req);
	bool validateHeaders(Request& req, bool& clearBufferOnError) const;

	std::string trimSpaces(const std::string& s) const;
	std::string toLower(const std::string& s) const;
	bool isDigits(const std::string& s) const;
	bool isValidHostValue(const std::string& s) const;
	bool isValidHeaderName(const std::string& s) const;
};

/* Responsibility:

read raw bytes
detect complete HTTP request
parse request line
parse headers
parse body
build a Request

It should not:

match locations
inspect files
know CGI logic
create responses

So HttpParser should answer only:

“Can I turn this byte stream into a valid Request object?” */
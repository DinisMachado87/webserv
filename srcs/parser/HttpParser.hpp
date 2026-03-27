/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/20 08:41:13 by akosloff          #+#    #+#             */
/*   Updated: 2026/03/27 14:17:18 by akosloff         ###   ########.fr       */
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

	Request* parse(char *rawBuffer, size_t bytesRead);

private:
	HttpParser(const HttpParser& other);
	HttpParser& operator=(const HttpParser& other);

	enum ChunkState {
		PARSE_SIZE,
		PARSE_DATA,
		PARSE_TRAILER,
		PARSE_DONE
	};

private:
	std::string _buffer;
	std::string _fullMessage;
	bool _isChunked;
	std::string _decodedBody;
	ChunkState _chunkState;

private:
	Request* makeErrorRequest(int code, const std::string& message);
	Request* eraseHeaderAndReturn(Request* err, size_t headerEnd);
	Request* clearBufferAndReturn(Request* err);

	bool firstLineParse(const std::string& firstLine, Request& req);
	bool headerParse(const std::string& headerLine, Request& req, bool& clearBufferOnError);
	bool parseRequestTarget(const std::string& target, Request& req);
	bool validateHeaders(Request& req, bool& clearBufferOnError) const;
	Request* parseChunkedBody(Request* req, size_t headerEnd);

	std::string trimSpaces(const std::string& s) const;
	std::string toLower(const std::string& s) const;
	bool isDigits(const std::string& s) const;
	bool isHexDigits(const std::string& s) const;
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
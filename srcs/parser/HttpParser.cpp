/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/20 08:41:23 by akosloff          #+#    #+#             */
/*   Updated: 2026/03/27 14:18:00 by akosloff         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpParser.hpp"
#include "../server/Server.hpp"
#include "../webServ.hpp"
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

HttpParser::HttpParser()
{
	_isChunked = false;
	_chunkState = PARSE_SIZE;
}

HttpParser::~HttpParser()
{
}

/* Parse one full HTTP request if enough bytes are already in _buffer.
Returns:
- NULL if request is not complete yet
- Request* if request is complete
- Request* with parse error set if malformed
*/
Request* HttpParser::parse(char *rawBuffer, size_t bytesRead)
{

	bool clearBufferOnError = false;

	if (rawBuffer != NULL && bytesRead > 0)
		_buffer.append(rawBuffer, bytesRead);

	/* First, wait until the full header block exists. */
	size_t headerEnd = _buffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		if (_buffer.size() > MAX_HEADER_SIZE) // limit to prevent DoS with huge headers now set to 8KB
		{
			_buffer.clear();
			return makeErrorRequest(431, "Request Header Fields Too Large");
		}
		return NULL;
	}

	Request* req = new Request();
	/* Parse only the header section first. */
	std::string headerBlock = _buffer.substr(0, headerEnd + 4);

	/* find end of first line and send to firstLineParse function */
	size_t lineEnd = headerBlock.find("\r\n");
	if (lineEnd == std::string::npos)
	{
		req->setParseError(400, "Bad request line");
		return eraseHeaderAndReturn(req, headerEnd);
	}

	std::string firstLine = headerBlock.substr(0, lineEnd);
	if (!firstLineParse(firstLine, *req))
		return eraseHeaderAndReturn(req, headerEnd);

	/* loop through all header lines and parse them */
	size_t current = lineEnd + 2;
	while (current < headerBlock.size())
	{
		size_t next = headerBlock.find("\r\n", current);
		if (next == std::string::npos)
		{
			req->setParseError(400, "Bad header line");
			return eraseHeaderAndReturn(req, headerEnd);
		}

		/* Empty line => end of headers */
		if (next == current)
			break;

		std::string headerLine = headerBlock.substr(current, next - current);
		if (!headerParse(headerLine, *req, clearBufferOnError))
		{
			if (clearBufferOnError)
				return clearBufferAndReturn(req);
			return eraseHeaderAndReturn(req, headerEnd);
		}

		current = next + 2;
	}

	if (!validateHeaders(*req, clearBufferOnError))
	{
		if (clearBufferOnError)
			return clearBufferAndReturn(req);
		return eraseHeaderAndReturn(req, headerEnd);
	}

	/* Handle chunked encoding */
	if (_isChunked)
		return parseChunkedBody(req, headerEnd);

	/* If there is a body, wait until all body bytes are present. */
	size_t totalNeeded = headerEnd + 4 + req->getContentLength();

	if (_buffer.size() < totalNeeded)
	{
		delete req;
		return NULL;
	}

	/* We have the full request, including body if any. */
	_fullMessage = _buffer.substr(0, totalNeeded);

	if (req->getContentLength() > 0)
		req->setBody(_buffer.substr(headerEnd + 4, req->getContentLength()));
	else
		req->setBody("");

	/* Remove only the current request from the buffer. */
	_buffer.erase(0, totalNeeded);

	return req;
}

/* Build an error Request object */
Request* HttpParser::makeErrorRequest(int code, const std::string& message)
{
	Request* req = new Request();
	req->setParseError(code, message);
	return req;
}

Request* HttpParser::eraseHeaderAndReturn(Request* err, size_t headerEnd)
{
	_buffer.erase(0, headerEnd + 4);
	return err;
}

Request* HttpParser::clearBufferAndReturn(Request* err)
{
	_buffer.clear();
	return err;
}

/* Parse the first request line: METHOD space REQUEST-TARGET space HTTP-VERSION */
bool HttpParser::firstLineParse(const std::string& firstLine, Request& req)
{
	size_t firstSpace = firstLine.find(' ');
	if (firstSpace == std::string::npos)
	{
		req.setParseError(400, "Bad request line: missing first space");
		return false;
	}

	size_t secondSpace = firstLine.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
	{
		req.setParseError(400, "Bad request line: missing second space");
		return false;
	}

	size_t thirdSpace = firstLine.find(' ', secondSpace + 1);
	if (thirdSpace != std::string::npos)
	{
		req.setParseError(400, "Bad request line: too many spaces");
		return false;
	}

	std::string method = firstLine.substr(0, firstSpace);
	std::string rawTarget = firstLine.substr(firstSpace + 1,
		secondSpace - firstSpace - 1);
	std::string version = firstLine.substr(secondSpace + 1);

	if (method.empty() || rawTarget.empty() || version.empty())
	{
		req.setParseError(400, "Bad request line: empty component");
		return false;
	}

	req.setMethod(method);
	req.setRequestTarget(rawTarget);
	req.setRequestVersion(version);

	if (method == "GET")
		req.setType(REQ_GET);
	else if (method == "POST")
		req.setType(REQ_POST);
	else if (method == "DELETE")
		req.setType(REQ_DELETE);
	else
	{
		req.setParseError(405, "Unsupported method");
		return false;
	}

	if (version != "HTTP/1.1" && version != "HTTP/1.0")
	{
		req.setParseError(505, "Unsupported HTTP version");
		return false;
	}

	return parseRequestTarget(rawTarget, req);
}

/* Parse one header line: Name: Value
Also detect Content-Length here. */
bool HttpParser::headerParse(const std::string& headerLine, Request& req, bool& clearBufferOnError)
{
	size_t firstColon = headerLine.find(':');
	if (firstColon == std::string::npos)
	{
		req.setParseError(400, "Bad header line: missing colon");
		return false;
	}

	std::string name = trimSpaces(headerLine.substr(0, firstColon));
	std::string value = trimSpaces(headerLine.substr(firstColon + 1));

	if (!isValidHeaderName(name))
	{
		req.setParseError(400, "Invalid header name");
		return false;
	}

	std::string lowerName = toLower(name);

	if (lowerName == "content-length")
	{
		if (req.hasContentLength())
		{
			clearBufferOnError = true;
			req.setParseError(400, "Duplicate Content-Length");
			return false;
		}

		if (!isDigits(value))
		{
			clearBufferOnError = true;
			req.setParseError(400, "Invalid Content-Length");
			return false;
		}

		std::istringstream iss(value);
		size_t len = 0;
		iss >> len;
		if (iss.fail())
		{
			clearBufferOnError = true;
			req.setParseError(400, "Invalid Content-Length");
			return false;
		}

		req.setContentLength(len);
		req.setHasContentLength(true);
	}
	else if (lowerName == "host")
	{
		if (!req.getHost().empty())
		{
			req.setParseError(400, "Duplicate Host header");
			return false;
		}

		if (!isValidHostValue(value))
		{
			req.setParseError(400, "Invalid Host header");
			return false;
		}

		req.setHost(value);
	}
	else if (lowerName == "content-type")
	{
		req.setContentType(value);
	}
	else if (lowerName == "transfer-encoding")
	{
		std::string lowerValue = toLower(value);
		if (lowerValue == "chunked")
		{
			_isChunked = true;
			req.setTransferEncoding("chunked");
		}
		else
		{
			clearBufferOnError = true;
			req.setParseError(501, "Unsupported Transfer-Encoding");
			return false;
		}
	}

	/* store normalized header name */
	req.addHeader(lowerName, value);
	return true;
}

std::string HttpParser::trimSpaces(const std::string& s) const
{
	size_t start = 0;
	size_t end = s.size();

	while (start < end && (s[start] == ' ' || s[start] == '\t'))
		start++;

	while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
		end--;

	return s.substr(start, end - start);
}

std::string HttpParser::toLower(const std::string& s) const
{
	std::string result = s;

	for (size_t i = 0; i < result.size(); i++)
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	return result;
}

bool HttpParser::isDigits(const std::string& s) const
{
	size_t i;

	if (s.empty())
		return false;
	i = 0;
	while (i < s.size())
	{
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			return false;
		i++;
	}
	return true;
}

/* Host validation */
bool HttpParser::isValidHostValue(const std::string& s) const
{
	size_t i;

	if (s.empty())
		return false;
	i = 0;
	while (i < s.size())
	{
		unsigned char c = static_cast<unsigned char>(s[i]);
		if (c <= 32 || c >= 127 || s[i] == '/')
			return false;
		i++;
	}
	return true;
}

/* Split request-target into path + query. */
bool HttpParser::parseRequestTarget(const std::string& target, Request& req)
{
	if (target.empty())
	{
		req.setParseError(400, "Empty request target");
		return false;
	}
	if (target[0] != '/')
	{
		req.setParseError(400, "Unsupported request target form");
		return false;
	}

	for (size_t index = 0; index < target.size(); index++)
	{
		unsigned char c = static_cast<unsigned char>(target[index]);
		if (c < 32 || c == 127 || c == ' ')
		{
			req.setParseError(400, "Invalid request target");
			return false;
		}
	}

	size_t qmark = target.find('?');
	if (qmark == std::string::npos)
	{
		req.setRequestPath(target);
		req.setQueryString("");
	}
	else
	{
		req.setRequestPath(target.substr(0, qmark));
		req.setQueryString(target.substr(qmark + 1));
		if (req.getRequestPath().empty())
			req.setRequestPath("/");
	}

	return true;
}

bool HttpParser::validateHeaders(Request& req, bool& clearBufferOnError) const
{
	if (req.getRequestVersion() == "HTTP/1.1")
	{
		if (req.getHost().empty())
		{
			req.setParseError(400, "Missing Host header");
			return false;
		}
	}

	if (req.getType() == REQ_POST)
	{
		if (!_isChunked && !req.hasContentLength())
		{
			clearBufferOnError = true;
			req.setParseError(411, "Content Length or Transfer-Encoding required");
			return false;
		}
	}

	if (req.hasContentLength())
	{
		if (req.getContentLength() > MAX_CONTENT_LENGTH)
		{
			clearBufferOnError = true;
			req.setParseError(413, "Content Too Large");
			return false;
		}
	}

	return true;
}

bool HttpParser::isValidHeaderName(const std::string& s) const
{
	size_t i;

	if (s.empty())
		return false;
	i = 0;
	while (i < s.size())
	{
		unsigned char c = static_cast<unsigned char>(s[i]);
		if (c <= 32 || c >= 127 || s[i] == ':')
			return false;
		i++;
	}
	return true;
}

bool HttpParser::isHexDigits(const std::string& s) const
{
	if (s.empty())
		return false;
	for (size_t i = 0; i < s.size(); i++)
	{
		unsigned char c = static_cast<unsigned char>(s[i]);
		if (!std::isxdigit(c))
			return false;
	}
	return true;
}

/* Parse chunked request body according to HTTP/1.1 chunked transfer encoding */
Request* HttpParser::parseChunkedBody(Request* req, size_t headerEnd)
{
	size_t pos = headerEnd + 4;

	while (pos < _buffer.size())
	{
		if (_chunkState == PARSE_SIZE)
		{
			/* Look for size line ending with \r\n */
			size_t lineEnd = _buffer.find("\r\n", pos);
			if (lineEnd == std::string::npos)
			{
				/* Check total accumulated body size */
				if (_decodedBody.size() > MAX_CONTENT_LENGTH)
				{
					_buffer.clear();
					_decodedBody.clear();
					_isChunked = false;
				_chunkState = PARSE_SIZE;
					req->setParseError(413, "Content Too Large");
					return req;
				}
				return NULL;
			}

			std::string sizeLine = _buffer.substr(pos, lineEnd - pos);
			if (!isHexDigits(sizeLine))
			{
				_buffer.clear();
				_decodedBody.clear();
				_isChunked = false;
				_chunkState = PARSE_SIZE;
				req->setParseError(400, "Invalid chunk size");
				return req;
			}

			size_t chunkSize = 0;
			std::istringstream iss(sizeLine);
			iss >> std::hex >> chunkSize;

			if (iss.fail())
			{
				_buffer.clear();
				_decodedBody.clear();
				_isChunked = false;
				_chunkState = PARSE_SIZE;
				req->setParseError(400, "Invalid chunk size");
				return req;
			}

			if (chunkSize == 0)
			{
				/* Last chunk, move to trailer parsing */
				_chunkState = PARSE_TRAILER;
				pos = lineEnd + 2;
			}
			else
			{
				/* Move to data parsing for this chunk */
				_chunkState = PARSE_DATA;
				pos = lineEnd + 2;
				/* Store the chunk size for the data phase */
				req->setChunkSize(chunkSize);
			}
		}
		else if (_chunkState == PARSE_DATA)
		{
			size_t chunkSize = req->getChunkSize();
			size_t available = _buffer.size() - pos;

			if (available < chunkSize + 2) /* +2 for \r\n after data */
				return NULL;

			/* Extract chunk data */
			_decodedBody.append(_buffer.substr(pos, chunkSize));

			/* Check total size */
			if (_decodedBody.size() > MAX_CONTENT_LENGTH)
			{
				_buffer.clear();
				_decodedBody.clear();
				_isChunked = false;
				_chunkState = PARSE_SIZE;
				req->setParseError(413, "Content Too Large");
				return req;
			}

			pos += chunkSize + 2; /* +2 for \r\n */
			_chunkState = PARSE_SIZE;
		}
		else if (_chunkState == PARSE_TRAILER)
		{
			/* Check if we're at an empty line (marks end of trailers) */
			if (pos + 1 < _buffer.size() && _buffer[pos] == '\r' && _buffer[pos + 1] == '\n')
			{
				/* End of chunked body */
				pos += 2;
				req->setBody(_decodedBody);
				_buffer.erase(0, pos);
				_decodedBody.clear();
				_isChunked = false;
				_chunkState = PARSE_SIZE;
				return req;
			}

			/* Otherwise skip this trailer header line */
			size_t lineEnd = _buffer.find("\r\n", pos);
			if (lineEnd == std::string::npos)
				return NULL;

			pos = lineEnd + 2;
		}
	}

	return NULL;
}
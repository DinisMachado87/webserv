/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpParser.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/20 08:41:23 by akosloff          #+#    #+#             */
/*   Updated: 2026/04/07 16:26:32 by akosloff         ###   ########.fr       */
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
	_headerEnd = std::string::npos;
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
	if (_headerEnd == std::string::npos)
		_headerEnd = _buffer.find("\r\n\r\n");
	if (_headerEnd == std::string::npos)
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
	/* find end of first line and send to firstLineParse function */
	size_t lineEnd = _buffer.find("\r\n"); // we can safely search for \r\n only within the header block,
	if (lineEnd == std::string::npos || lineEnd > _headerEnd) // if no \r\n or if it's after the header end, it's a bad request
	{
		req->setParseError(400, "Bad request line");
		return eraseHeaderAndReturn(req, _headerEnd);
	}
	if (!firstLineParse(0, lineEnd, *req))
		return eraseHeaderAndReturn(req, _headerEnd);
	

	/* loop through all header lines and parse them */
	size_t current = lineEnd + 2;
	while (current < _headerEnd + 4)
	{

		size_t next = _buffer.find("\r\n", current);
		if (next == std::string::npos)
		{
			req->setParseError(400, "Bad header line");
			return eraseHeaderAndReturn(req, _headerEnd);
		}

		/* Empty line => end of headers */
		if (next == current)
			break;

		if (!headerParse(current, next - current, *req, clearBufferOnError))
		{
			if (clearBufferOnError)
				return clearBufferAndReturn(req);
			return eraseHeaderAndReturn(req, _headerEnd);
		}

		current = next + 2;
	}

	if (!validateHeaders(*req, clearBufferOnError))
	{
		if (clearBufferOnError)
			return clearBufferAndReturn(req);
		return eraseHeaderAndReturn(req, _headerEnd);
	}

	/* Handle chunked encoding */
	if (_isChunked)
		return parseChunkedBody(req, _headerEnd);

	/* If there is a body, wait until all body bytes are present. */
	size_t totalNeeded = _headerEnd + 4 + req->getContentLength();
	
	if (_buffer.size() < totalNeeded)
	{
		delete req;
		return NULL;
	}

	/* We have the full request, including body if any. */
	_fullMessage = _buffer.substr(0, totalNeeded);

	if (req->getContentLength() > 0)
		req->setBody(_buffer.substr(_headerEnd + 4, req->getContentLength()));
	else
		req->setBody("");

	/* Remove only the current request from the buffer. */
	_buffer.erase(0, totalNeeded);
	
	return req;
}

/* Parse the first request line: METHOD space REQUEST-TARGET space HTTP-VERSION */
bool HttpParser::firstLineParse(size_t startLine, size_t length, Request& req)
{
	size_t lineEnd = startLine + length;
	
	size_t firstSpace = _buffer.find(" ", startLine);
	if (firstSpace == std::string::npos || firstSpace >= lineEnd)
	{
		req.setParseError(400, "Bad request line: missing first space");
		return false;
	}

	size_t secondSpace = _buffer.find(" ", firstSpace + 1);
	if (secondSpace == std::string::npos || secondSpace >= lineEnd)
	{
		req.setParseError(400, "Bad request line: missing second space");
		return false;
	}

	size_t thirdSpace = _buffer.find(" ", secondSpace + 1);
	if (thirdSpace != std::string::npos && thirdSpace < lineEnd)
	{
		req.setParseError(400, "Bad request line: too many spaces");
		return false;
	}

	// Calculate component lengths (no copies)
	size_t methodLen = firstSpace - startLine;
	size_t targetLen = secondSpace - firstSpace - 1;
	size_t versionStart = secondSpace + 1;
	size_t versionLen = length - (secondSpace - startLine + 1);
	
	// Validate components exist
	if (methodLen == 0 || targetLen == 0 || versionLen == 0)
	{
		req.setParseError(400, "Bad request line: empty component");
		return false;
	}

	// Direct buffer comparisons for method type (no copies)
	if (methodLen == 3 && _buffer.compare(startLine, 3, "GET") == 0)
		req.setType(REQ_GET);
	else if (methodLen == 4 && _buffer.compare(startLine, 4, "POST") == 0)
		req.setType(REQ_POST);
	else if (methodLen == 6 && _buffer.compare(startLine, 6, "DELETE") == 0)
		req.setType(REQ_DELETE);
	else
	{
		req.setParseError(405, "Unsupported method");
		return false;
	}

	// Version check (no copies)
	bool validVersion = (versionLen == 8 && _buffer.compare(versionStart, 8, "HTTP/1.1") == 0) ||
	                    (versionLen == 8 && _buffer.compare(versionStart, 8, "HTTP/1.0") == 0);
	if (!validVersion)
	{
		req.setParseError(505, "Unsupported HTTP version");
		return false;
	}

	// Only create strings when actually setting them
	req.setMethod(std::string(_buffer.data() + startLine, methodLen));
	req.setRequestTarget(std::string(_buffer.data() + firstSpace + 1, targetLen));
	req.setRequestVersion(std::string(_buffer.data() + versionStart, versionLen));

	// Create rawTarget for parseRequestTarget
	std::string rawTarget(_buffer.data() + firstSpace + 1, targetLen);
	return parseRequestTarget(rawTarget, req);
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



/* Parse one header line: Name: Value
Also detect Content-Length here. */
bool HttpParser::headerParse(size_t startLine, size_t length, Request& req, bool& clearBufferOnError)
{
	// Find colon in the header line within buffer
	size_t colonPos = _buffer.find(':', startLine);
	if (colonPos == std::string::npos || colonPos >= startLine + length)
	{
		req.setParseError(400, "Bad header line: missing colon");
		return false;
	}

	// Get trimmed bounds for name and value (no string copies)
	size_t nameStart = startLine;
	size_t nameEnd = colonPos;
	trimSpacesBounds(nameStart, nameEnd);

	size_t valueStart = colonPos + 1;
	size_t valueEnd = startLine + length;
	trimSpacesBounds(valueStart, valueEnd);

	if (nameStart >= nameEnd)
	{
		req.setParseError(400, "Invalid header name");
		return false;
	}

	// Validate header name using buffer range (no copy)
	if (!isValidHeaderNameRange(nameStart, nameEnd))
	{
		req.setParseError(400, "Invalid header name");
		return false;
	}

	// Create lowercase name string inline (one copy only)
	std::string name(_buffer.data() + nameStart, nameEnd - nameStart);
	toLowerCase(name);

	// Create value string inline (one copy only)
	std::string value(_buffer.data() + valueStart, valueEnd - valueStart);

	if (name == "content-length")
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
	else if (name == "host")
	{
		if (!req.getHost().empty())
		{
			req.setParseError(400, "Duplicate Host header");
			return false;
		}

		if (!isValidHostValueRange(valueStart, valueEnd))
		{
			req.setParseError(400, "Invalid Host header");
			return false;
		}

		req.setHost(value);
	}
	else if (name == "content-type")
	{
		req.setContentType(value);
	}
	else if (name == "transfer-encoding")
	{
		toLowerCase(value);
		if (value == "chunked")
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

	/*  normalized header name */
	req.addHeader(name, value);
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

/* Get trimmed bounds from buffer positions without creating a string copy */
void HttpParser::trimSpacesBounds(size_t& start, size_t& end) const
{
	/* Safety clamp first */
	size_t bufSize = _buffer.size();
	if (start > bufSize)
		start = bufSize;
	if (end > bufSize)
		end = bufSize;

	/* Trim leading whitespace */
	while (start < end && (start < bufSize) && (_buffer[start] == ' ' || _buffer[start] == '\t'))
		start++;

	/* Trim trailing whitespace */
	while (end > start && (end > 0) && (_buffer[end - 1] == ' ' || _buffer[end - 1] == '\t'))
		end--;
}

/* std::string HttpParser::trimSpaces(size_t startPos, size_t length) const
{
	size_t end = startPos + length;
	trimSpacesBounds(startPos, end);
	return _buffer.substr(startPos, end - startPos);
} */

void HttpParser::toLowerCase(std::string& s)
{

    for (std::string::iterator it = s.begin(); it != s.end(); ++it)
       *it = std::tolower(static_cast<unsigned char>(*it));
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

/* Host validation for buffer range (no copy needed) */
bool HttpParser::isValidHostValueRange(size_t start, size_t end) const
{
	if (start >= end || end > _buffer.size())
		return false;

	for (size_t i = start; i < end; i++)
	{
		unsigned char c = static_cast<unsigned char>(_buffer[i]);
		if (c <= 32 || c >= 127 || _buffer[i] == '/')
			return false;
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

/* Header name validation for buffer range (no copy needed) */
bool HttpParser::isValidHeaderNameRange(size_t start, size_t end) const
{
	if (start >= end || end > _buffer.size())
		return false;

	for (size_t i = start; i < end; i++)
	{
		unsigned char c = static_cast<unsigned char>(_buffer[i]);
		if (c <= 32 || c >= 127 || _buffer[i] == ':')
			return false;
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

bool HttpParser::isHexDigits(size_t startPos, size_t length) const
{
	if (length == 0 || startPos + length > _buffer.size())
		return false;
	for (size_t i = 0; i < length; i++)
	{
		unsigned char c = static_cast<unsigned char>(_buffer[startPos + i]);
		if (!std::isxdigit(c))
			return false;
	}
	return true;
}

/* Parse hex string from buffer range directly without creating substring */
bool HttpParser::hexStringToSize(size_t start, size_t length, size_t& result) const
{
	if (length == 0 || start + length > _buffer.size())
		return false;

	result = 0;
	for (size_t i = 0; i < length; i++)
	{
		unsigned char c = static_cast<unsigned char>(_buffer[start + i]);
		if (!std::isxdigit(c))
			return false;

		int digit = (c >= '0' && c <= '9') ? (c - '0') :
		            (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
		            (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : -1;
		if (digit < 0)
			return false;

		result = (result << 4) | digit;
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

			size_t chunkSize = 0;
			if (!hexStringToSize(pos, lineEnd - pos, chunkSize))
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

			/* Extract chunk data (no intermediate copy) */
			_decodedBody.append(_buffer.data() + pos, chunkSize);

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
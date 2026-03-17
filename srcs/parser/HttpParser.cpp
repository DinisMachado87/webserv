#include "HttpParser.hpp"

HttpParser::HttpParser()
{
	std::cout << "HttpParser constructed\n";
}

HttpParser::~HttpParser() {}

/* Reset request state before parsing a new request */
void HttpParser::resetReqVariables(int clientFD)
{
	_reqVariables = reqVariables();
	_reqVariables.headers.clear();
	_reqVariables.method.clear();
	_reqVariables.requestPath.clear();
	_reqVariables.requestVersion.clear();
	_reqVariables.queryString.clear();
	_reqVariables.host.clear();
	_reqVariables.body.clear();
	_reqVariables.clientFD = clientFD;
	_reqVariables.errorCode = 0;
	_reqVariables.errorMessage = "";
	_reqVariables.type = REQ_GET;
	_reqVariables.contentLength = 0;
	_reqVariables.hasContentLength = false;
	_reqVariables.clearBufferOnError = false;
	_fullMessage.clear();
}
/* Parse one full HTTP request if enough bytes are already in _buffer.
Returns:
- NULL if request is not complete yet
- Request* if request is complete
- Request* with error fields if malformed
*/
Request* HttpParser::parse(char *rawBuffer, size_t bytesRead, int clientFD)
{
	Request* errorReq = NULL;

	if (rawBuffer != NULL && bytesRead > 0)
		_buffer.append(rawBuffer, bytesRead);


	resetReqVariables(clientFD);
	/* First, wait until the full header block exists.	*/
	size_t headerEnd = _buffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		if (_buffer.size() > 8192) //safety check to reject big buffers.
		{
			_buffer.clear();
			return makeErrorRequest(431, "Request Header Fields Too Large", REQ_ERROR);
		}
		return NULL;
	}

	/*Parse only the header section first. loops and calls headerParse function*/
	std::string headerBlock = _buffer.substr(0, headerEnd + 4);



	/* find end of first line and send to firstLineParse function */
	size_t lineEnd = headerBlock.find("\r\n");
	if (lineEnd == std::string::npos)
		return eraseHeaderAndReturn(makeErrorRequest(400, "Bad request line", REQ_ERROR), headerEnd);

	std::string firstLine = headerBlock.substr(0, lineEnd);
	if ((errorReq = firstLineParse(firstLine)) != NULL)
		return eraseHeaderAndReturn(errorReq, headerEnd);

	/* Parse all header lines */
	size_t current = lineEnd + 2;
	while (current < headerBlock.size())
	{
		size_t next = headerBlock.find("\r\n", current);
		if (next == std::string::npos)
			return eraseHeaderAndReturn(makeErrorRequest(400, "Bad header line", REQ_ERROR), headerEnd);
		/* Empty line => end of headers	*/
		if (next == current)
			break;

		std::string headerLine = headerBlock.substr(current, next - current);
		if ((errorReq = headerParse(headerLine)) != NULL)
		{
			if (_reqVariables.clearBufferOnError)
				return clearBufferAndReturn(errorReq);
			return eraseHeaderAndReturn(errorReq, headerEnd);
		}

		current = next + 2;
	}
	if ((errorReq = validateHeaders()) != NULL)
	{
		if (_reqVariables.clearBufferOnError)
			return clearBufferAndReturn(errorReq);
		return eraseHeaderAndReturn(errorReq, headerEnd);
	}
	/* If there is a body content length will be set,
	wait until all body bytes are present.
	totalNeeded = bytes of headers including "\r\n\r\n" + bytes of body from Content-Length	*/
	size_t totalNeeded = headerEnd + 4 + static_cast<size_t>(_reqVariables.contentLength);

	if (_buffer.size() < totalNeeded)
		return NULL;

	/* we have the full request, including body if any.	*/
	_fullMessage = _buffer.substr(0, totalNeeded);

	if (_reqVariables.contentLength > 0)
		_reqVariables.body = _buffer.substr(headerEnd + 4, static_cast<size_t>(_reqVariables.contentLength));
	else
		_reqVariables.body.clear();

	/* Remove only the current request from the buffer. This is important if multiple requests arrived together.*/
	_buffer.erase(0, totalNeeded);

	reqVariables *currentVars = new reqVariables;
	*currentVars = _reqVariables;
	return new Request(currentVars);
}

/* Build an error Request object */
Request* HttpParser::makeErrorRequest(int code, const std::string& message, e_request_type type)
{
	_reqVariables.errorCode = code;
	_reqVariables.errorMessage = message;
	_reqVariables.type = type;

	reqVariables *currentVars = new reqVariables;
	*currentVars = _reqVariables;
	return new Request(currentVars);
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

/* Parse the first request line: METHOD space REQUEST-TARGET spce HTTP-VERSION */
Request* HttpParser::firstLineParse(const std::string& firstLine)
{
	size_t firstSpace = firstLine.find(' ');
	if (firstSpace == std::string::npos)
		return makeErrorRequest(400, "Bad request line: missing first space", REQ_ERROR);

	size_t secondSpace = firstLine.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
		return makeErrorRequest(400, "Bad request line: missing second space", REQ_ERROR);

	size_t thirdSpace = firstLine.find(' ', secondSpace + 1); //check if this is not too strict, maybe it's ok to accept a trailing space
	if (thirdSpace != std::string::npos)
		return makeErrorRequest(400, "Bad request line: too many spaces", REQ_ERROR);

	_reqVariables.method = firstLine.substr(0, firstSpace);
	std::string rawTarget = firstLine.substr(firstSpace + 1,
		secondSpace - firstSpace - 1);
	_reqVariables.requestVersion = firstLine.substr(secondSpace + 1);

	if (_reqVariables.method.empty() || rawTarget.empty()
	|| _reqVariables.requestVersion.empty())
		return makeErrorRequest(400, "Bad request line: empty component", REQ_ERROR);

	if (_reqVariables.method == "GET")
		_reqVariables.type = REQ_GET;
	else if (_reqVariables.method == "POST")
		_reqVariables.type = REQ_POST;
	else if (_reqVariables.method == "DELETE")
		_reqVariables.type = REQ_DELETE;
	else
		return makeErrorRequest(405, "Unsupported method", REQ_ERROR);

	if (_reqVariables.requestVersion != "HTTP/1.1"
		&& _reqVariables.requestVersion != "HTTP/1.0")
		return makeErrorRequest(505, "Unsupported HTTP version", REQ_ERROR);

	return parseRequestTarget(rawTarget);
}


/* Parse one header line: Name: Value
Also detect Content-Length here. */
Request* HttpParser::headerParse(const std::string& headerLine)
{
	size_t firstColon = headerLine.find(':');
	if (firstColon == std::string::npos)
		return makeErrorRequest(400, "Bad header line: missing colon", REQ_ERROR);

	HeaderField temp;
	temp.name = trimSpaces(headerLine.substr(0, firstColon));
	temp.value = trimSpaces(headerLine.substr(firstColon + 1));

	if (!isValidHeaderName(temp.name))
		return makeErrorRequest(400, "Invalid header name", REQ_ERROR);

	std::string lowerName = toLower(temp.name);

	if (lowerName == "content-length")
	{
		if (_reqVariables.hasContentLength)
		{
			_reqVariables.clearBufferOnError = true;
			return makeErrorRequest(400, "Duplicate Content-Length", REQ_ERROR);
		}

		if (!isDigits(temp.value))
		{
			_reqVariables.clearBufferOnError = true;
			return makeErrorRequest(400, "Invalid Content-Length", REQ_ERROR);
		}

		std::istringstream iss(temp.value); //get content length from string using stream
		size_t len = 0;
		iss >> len;
		if (iss.fail())
		{
			_reqVariables.clearBufferOnError = true;
			return makeErrorRequest(400, "Invalid Content-Length", REQ_ERROR);
		}

		_reqVariables.contentLength = len;
		_reqVariables.hasContentLength = true;
	}
	else if (lowerName == "host")
	{
		if (!_reqVariables.host.empty())
			return makeErrorRequest(400, "Duplicate Host header", REQ_ERROR);

		if (!isValidHostValue(temp.value))
			return makeErrorRequest(400, "Invalid Host header", REQ_ERROR);

		_reqVariables.host = temp.value;
	}

	/*
	** For now reject Transfer-Encoding because you probably do not support chunked bodies yet.
	*/
	else if (lowerName == "transfer-encoding")
	{
		_reqVariables.clearBufferOnError = true;
		return makeErrorRequest(501, "Transfer-Encoding not supported", REQ_ERROR);
	}

	_reqVariables.headers.push_back(temp);
	return NULL;
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
Request* HttpParser::parseRequestTarget(const std::string& target)
{
	size_t qmark;

	if (target.empty())
		return makeErrorRequest(400, "Empty request target", REQ_ERROR);

	if (target[0] != '/')
		return makeErrorRequest(400, "Unsupported request target form", REQ_ERROR);

	qmark = target.find('?');
	if (qmark == std::string::npos)
	{
		_reqVariables.requestPath = target;
		_reqVariables.queryString.clear();
	}
	else
	{
		_reqVariables.requestPath = target.substr(0, qmark);
		_reqVariables.queryString = target.substr(qmark + 1);
		if (_reqVariables.requestPath.empty())
			_reqVariables.requestPath = "/";
	}

	return NULL;
}

Request* HttpParser::validateHeaders(void)
{
	/* HTTP/1.1 requires Host. */
	if (_reqVariables.requestVersion == "HTTP/1.1")
	{
		if (_reqVariables.host.empty())
			return makeErrorRequest(400, "Missing Host header", REQ_ERROR);
	}

	/* rejects POST request without content length */
	if (_reqVariables.type == REQ_POST && !_reqVariables.hasContentLength)
	{
		_reqVariables.clearBufferOnError = true;
		return makeErrorRequest(411, "Content Length required", REQ_ERROR);
	}
	/* Body size limit. If Content-Length says the body will be too large, reject immediately with 413.	*/
	if (_reqVariables.hasContentLength)
	{
		if (static_cast<size_t>(_reqVariables.contentLength) > 1024)
		{
			_reqVariables.clearBufferOnError = true;
			return makeErrorRequest(413, "Content Too Large", REQ_ERROR);
		}
	}

	return NULL;
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
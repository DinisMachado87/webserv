#include "HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser() :
	_state(STATE_REQUEST_LINE),
	_request(),
	_errorCode(0),
	_errorMessage(),
	_contentLength(0),
	_maxBodySize(1024 * 1024)
{
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser& other) :
	_state(other._state),
	_request(other._request),
	_errorCode(other._errorCode),
	_errorMessage(other._errorMessage),
	_contentLength(other._contentLength),
	_maxBodySize(other._maxBodySize)
{
}

HttpRequestParser& HttpRequestParser::operator=(const HttpRequestParser& other)
{
	if (this == &other)
		return *this;

	_state = other._state;
	_request = other._request;
	_errorCode = other._errorCode;
	_errorMessage = other._errorMessage;
	_contentLength = other._contentLength;
	_maxBodySize = other._maxBodySize;

	return *this;
}

HttpRequestParser::~HttpRequestParser()
{
}

void HttpRequestParser::reset()
{
	_state = STATE_REQUEST_LINE;
	_request.clear();
	_errorCode = 0;
	_errorMessage.clear();
	_contentLength = 0;
}

const HttpRequest& HttpRequestParser::getRequest() const
{
	return _request;
}

int HttpRequestParser::getErrorCode() const
{
	return _errorCode;
}

const std::string& HttpRequestParser::getErrorMessage() const
{
	return _errorMessage;
}

HttpRequestParser::State HttpRequestParser::getState() const
{
	return _state;
}

void HttpRequestParser::setMaxBodySize(size_t maxBodySize)
{
	_maxBodySize = maxBodySize;
}

void HttpRequestParser::setError(int code, const std::string& message)
{
	_state = STATE_ERROR;
	_errorCode = code;
	_errorMessage = message;
}

size_t HttpRequestParser::findCRLF(const std::string& s) const
{
	for (size_t i = 0; i + 1 < s.size(); ++i)
	{
		if (s[i] == '\r' && s[i + 1] == '\n')
			return i;
	}
	return std::string::npos;
}

size_t HttpRequestParser::findChar(const std::string& s, char c, size_t start, size_t end) const
{
	for (size_t i = start; i < end; ++i)
	{
		if (s[i] == c)
			return i;
	}
	return std::string::npos;
}

std::string HttpRequestParser::trim(const std::string& s) const
{
	size_t start = 0;
	size_t end = s.size();

	while (start < end && (s[start] == ' ' || s[start] == '\t'))
		start++;

	while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
		end--;

	return s.substr(start, end - start);
}

RequestMethod HttpRequestParser::parseMethod(const std::string& method) const
{
	if (method == "GET")
		return METHOD_GET;
	if (method == "POST")
		return METHOD_POST;
	if (method == "DELETE")
		return METHOD_DELETE;
	return METHOD_UNKNOWN;
}

bool HttpRequestParser::isSupportedVersion(const std::string& version) const
{
	return (version == "HTTP/1.1" || version == "HTTP/1.0");
}

bool HttpRequestParser::parseContentLength(const std::string& value, size_t& out) const
{
	if (value.empty())
		return false;

	size_t result = 0;
	for (size_t i = 0; i < value.size(); ++i)
	{
		if (value[i] < '0' || value[i] > '9')
			return false;
		result = result * 10 + static_cast<size_t>(value[i] - '0');
	}
	out = result;
	return true;
}

void HttpRequestParser::splitTarget()
{
	size_t q = _request.target.find('?');
	if (q == std::string::npos)
	{
		_request.path = _request.target;
		_request.queryString.clear();
	}
	else
	{
		_request.path = _request.target.substr(0, q);
		_request.queryString = _request.target.substr(q + 1);
	}
}

HttpRequestParser::Result HttpRequestParser::parseRequestLine(std::string& buffer)
{
	size_t lineEnd = findCRLF(buffer);
	if (lineEnd == std::string::npos)
		return NEED_MORE;

	std::string line = buffer.substr(0, lineEnd);
	buffer.erase(0, lineEnd + 2);

	size_t sp1 = findChar(line, ' ', 0, line.size());
	if (sp1 == std::string::npos)
	{
		setError(400, "Bad request line: missing first space");
		return PARSE_ERROR;
	}

	size_t sp2 = findChar(line, ' ', sp1 + 1, line.size());
	if (sp2 == std::string::npos)
	{
		setError(400, "Bad request line: missing second space");
		return PARSE_ERROR;
	}

	if (findChar(line, ' ', sp2 + 1, line.size()) != std::string::npos)
	{
		setError(400, "Bad request line: too many spaces");
		return PARSE_ERROR;
	}

	_request.methodString = line.substr(0, sp1);
	_request.method = parseMethod(_request.methodString);
	_request.target = line.substr(sp1 + 1, sp2 - sp1 - 1);
	_request.version = line.substr(sp2 + 1);

	if (_request.methodString.empty() || _request.target.empty() || _request.version.empty())
	{
		setError(400, "Bad request line: empty component");
		return PARSE_ERROR;
	}

	if (_request.method == METHOD_UNKNOWN)
	{
		setError(405, "Unsupported method");
		return PARSE_ERROR;
	}

	if (!isSupportedVersion(_request.version))
	{
		setError(505, "Unsupported HTTP version");
		return PARSE_ERROR;
	}

	splitTarget();
	_state = STATE_HEADERS;
	return NEED_MORE;
}

HttpRequestParser::Result HttpRequestParser::parseHeaders(std::string& buffer)
{
	while (1)
	{
		size_t lineEnd = findCRLF(buffer);
		if (lineEnd == std::string::npos)
			return NEED_MORE;

		if (lineEnd == 0)
		{
			buffer.erase(0, 2);

			if (_contentLength > 0)
				_state = STATE_BODY;
			else
				_state = STATE_DONE;

			return (_state == STATE_DONE) ? PARSE_DONE : NEED_MORE;
		}

		std::string line = buffer.substr(0, lineEnd);
		buffer.erase(0, lineEnd + 2);

		size_t colon = findChar(line, ':', 0, line.size());
		if (colon == std::string::npos)
		{
			setError(400, "Bad header: missing colon");
			return PARSE_ERROR;
		}

		HeaderField header;
		header.name = trim(line.substr(0, colon));
		header.value = trim(line.substr(colon + 1));

		if (header.name.empty())
		{
			setError(400, "Bad header: empty name");
			return PARSE_ERROR;
		}

		_request.headers.push_back(header);

		if (HttpRequest::toLower(header.name) == "content-length")
		{
			size_t parsedLen = 0;
			if (!parseContentLength(header.value, parsedLen))
			{
				setError(400, "Invalid Content-Length");
				return PARSE_ERROR;
			}

			if (parsedLen > _maxBodySize)
			{
				setError(413, "Request body too large");
				return PARSE_ERROR;
			}

			_contentLength = parsedLen;
			_request.contentLength = static_cast<int>(parsedLen);
		}
	}

	/* unreachable */
	return NEED_MORE;
}

HttpRequestParser::Result HttpRequestParser::parseBody(std::string& buffer)
{
	if (buffer.size() < _contentLength)
		return NEED_MORE;

	_request.body = buffer.substr(0, _contentLength);
	buffer.erase(0, _contentLength);

	_state = STATE_DONE;
	return PARSE_DONE;
}

HttpRequestParser::Result HttpRequestParser::feed(std::string& buffer)
{
	while (1)
	{
		if (_state == STATE_REQUEST_LINE)
		{
			Result r = parseRequestLine(buffer);
			if (r == PARSE_ERROR)
				return PARSE_ERROR;
			if (_state != STATE_HEADERS)
				return NEED_MORE;
		}
		else if (_state == STATE_HEADERS)
		{
			Result r = parseHeaders(buffer);
			if (r == PARSE_ERROR)
				return PARSE_ERROR;
			if (r == PARSE_DONE)
				return PARSE_DONE;
			if (_state != STATE_BODY)
				return NEED_MORE;
		}
		else if (_state == STATE_BODY)
		{
			return parseBody(buffer);
		}
		else if (_state == STATE_DONE)
		{
			return PARSE_DONE;
		}
		else
		{
			return PARSE_ERROR;
		}
	}
}
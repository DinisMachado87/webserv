#include "HttpRequestParser.hpp"

/* ------------------------------------------------------------------
   Default constructor

   Initializes the parser in its starting state:
   - waiting for the request line
   - empty parsed request object
   - no error
   - no Content-Length seen yet
   - default maximum body size = 1 MB

  one parser object should belong to one client connection.
------------------------------------------------------------------- */
HttpRequestParser::HttpRequestParser() :
	_state(STATE_REQUEST_LINE),
	_request(),
	_errorCode(0),
	_errorMessage(),
	_contentLength(0),
	_maxBodySize(1024 * 1024)
{
}

//   Copy constructor
HttpRequestParser::HttpRequestParser(const HttpRequestParser& other) :
	_state(other._state),
	_request(other._request),
	_errorCode(other._errorCode),
	_errorMessage(other._errorMessage),
	_contentLength(other._contentLength),
	_maxBodySize(other._maxBodySize)
{
}

//   Copy assignment operator
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

/* ------------------------------------------------------------------
   reset()
   Resets the parser so it can start parsing a new HTTP request.
   This is used after:
   - a full request has been successfully handled, or
   - the connection wants to continue with keep-alive
------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------
   setMaxBodySize()
   Sets the maximum allowed request body size.
   This protects the server from huge uploads or malicious requests.
   If Content-Length exceeds this value, parsing fails with 413.
------------------------------------------------------------------- */
void HttpRequestParser::setMaxBodySize(size_t maxBodySize)
{
	_maxBodySize = maxBodySize;
}

/* ------------------------------------------------------------------
   setError()
   Moves the parser into STATE_ERROR and stores both:
   - an error code
   - an explanatory message
   This centralizes error handling so parse functions can fail cleanly.
------------------------------------------------------------------- */
void HttpRequestParser::setError(int code, const std::string& message)
{
	_state = STATE_ERROR;
	_errorCode = code;
	_errorMessage = message;
}

/* ------------------------------------------------------------------
   findCRLF()
   Searches for the first "\r\n" sequence in a string.

   HTTP uses CRLF as the line ending, so this helper is used to find:
   - the end of the request line
   - the end of each header line

   Returns:
   - index of '\r' if found
   - std::string::npos otherwise
------------------------------------------------------------------- */
size_t HttpRequestParser::findCRLF(const std::string& s) const
{
	for (size_t i = 0; i + 1 < s.size(); ++i)
	{
		if (s[i] == '\r' && s[i + 1] == '\n')
			return i;
	}
	return std::string::npos;
}

/* ------------------------------------------------------------------
   findChar()

   Searches for a single character inside a substring range:
   [start, end)

   Used for:
   - finding spaces in the request line
   - finding ':' in header lines

   Returns:
   - index of the character if found
   - std::string::npos otherwise
------------------------------------------------------------------- */
size_t HttpRequestParser::findChar(const std::string& s, char c, size_t start, size_t end) const
{
	for (size_t i = start; i < end; ++i)
	{
		if (s[i] == c)
			return i;
	}
	return std::string::npos;
}

/* ------------------------------------------------------------------
   trim()

   Removes leading and trailing spaces/tabs from a string.

   Used mainly for header parsing so that:
   "Content-Type:   text/html   "
   becomes:
   "text/html"
------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------
   parseMethod()

   Converts the method text from the request line into the internal enum.

   Supported methods:
   - GET
   - POST
   - DELETE

   Any other method becomes METHOD_UNKNOWN and will later trigger 405.
------------------------------------------------------------------- */
RequestMethod HttpRequestParser::parseMethod(const std::string& method) const
{
	switch (method[0])
	{
		case 'G':
			if (method == "GET")
				return METHOD_GET;
			break;

		case 'P':
			if (method == "POST")
				return METHOD_POST;
			break;

		case 'D':
			if (method == "DELETE")
				return METHOD_DELETE;
			break;
	}

	return METHOD_UNKNOWN;
}

bool HttpRequestParser::isSupportedVersion(const std::string& version) const
{
	return (version == "HTTP/1.1" || version == "HTTP/1.0");
}

/* ------------------------------------------------------------------
   parseContentLength()

   Parses the Content-Length header value into a numeric size_t.

   Rules:
   - empty string is invalid
   - only decimal digits are accepted
   - no signs, spaces, or other characters are accepted

   Returns:
   - true on success, with 'out' filled
   - false on invalid input
------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------
   splitTarget()

   Splits the raw request target into:
   - path
   - query string

   Example:
   target = "/search?q=test&x=1"

   becomes:
   path        = "/search"
   queryString = "q=test&x=1"

   If no '?' exists, the whole target becomes the path and
   queryString is cleared.
------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------
   parseRequestLine()

   Parses the first line of the HTTP request, for example:

   "GET /hello HTTP/1.1\r\n"

   Steps:
   1. wait until a full line ending in CRLF exists
   2. remove that line from the buffer
   3. split it into 3 parts:
      - method
      - target
      - version
   4. validate method and version
   5. split target into path + query string
   6. move parser state to STATE_HEADERS

   Returns:
   - NEED_MORE if the line is incomplete
   - PARSE_ERROR on malformed request line
   - NEED_MORE again after successful parse, because next state is HEADERS
------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------
   parseHeaders()

   Parses header lines one by one until it reaches the empty line that
   marks the end of headers.

   Example header:
   "Host: localhost\r\n"

   Steps:
   1. repeatedly look for CRLF
   2. if line is empty, headers are done
   3. otherwise split line on ':'
   4. trim header name and value
   5. store header in _request.headers
   6. if header is Content-Length, parse and validate it

   When headers end:
   - if Content-Length > 0, go to STATE_BODY
   - otherwise go directly to STATE_DONE

   Returns:
   - NEED_MORE if headers are incomplete
   - PARSE_DONE if full request is complete with no body
   - PARSE_ERROR if a header is malformed
------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------
   parseBody()

   Parses the request body when Content-Length was present and > 0.

   Steps:
   1. wait until the buffer contains at least _contentLength bytes
   2. copy exactly that many bytes into _request.body
   3. erase consumed body bytes from the buffer
   4. move to STATE_DONE

   Returns:
   - NEED_MORE if body is still incomplete
   - PARSE_DONE when the full body has been read
------------------------------------------------------------------- */
HttpRequestParser::Result HttpRequestParser::parseBody(std::string& buffer)
{
	if (buffer.size() < _contentLength)
		return NEED_MORE;

	_request.body = buffer.substr(0, _contentLength);
	buffer.erase(0, _contentLength);

	_state = STATE_DONE;
	return PARSE_DONE;
}

/* ------------------------------------------------------------------
   feed()

   This is the main driver of the parser state machine.

   It examines the current state and delegates to the correct parsing
   function:
   - request line
   - headers
   - body

   Because socket data may arrive in pieces, feed() can be called many
   times on the same client buffer.

   Important behavior:
   - it consumes bytes from 'buffer' as parsing succeeds
   - it stops when:
     * more bytes are needed
     * parsing completed
     * a parse error occurred

   Returns:
   - NEED_MORE  : parser needs more input bytes
   - PARSE_DONE : one full request has been parsed
   - PARSE_ERROR: malformed request / unsupported method / etc.
------------------------------------------------------------------- */
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
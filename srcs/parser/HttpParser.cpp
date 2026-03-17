#include "HttpParser.hpp"

HttpParser::HttpParser()
{
	std::cout << "HttpParser constructed\n";
}

HttpParser::~HttpParser() {}

/* here I should set an error code, without it this function is useless */
Request* HttpParser::makeErrorRequest(int Code, std::string Message, e_response_type Type)
{
	_reqVariables.errorCode = Code;
	_reqVariables.errorMessage = Message;
		_reqVariables.type = Type;
	reqVariables *currentVars = new reqVariables;
	*currentVars = _reqVariables;
	return new Request(currentVars);
}

Request* HttpParser::parse(char *rawBuffer, size_t bitesRead, int clientFD)
{
	Request* errorReq = NULL;
	_buffer.append(rawBuffer, bitesRead);

	/* if there is no CRLF we return to the connection class function handleIn() 
	maybe here to use end() will save time of iterating the whole string
	what happens if inside the buffer I got there is 2 reuqeusts? same problems as getNextLine*/
	size_t reqEnd = _buffer.find("\r\n\r\n");
	if (reqEnd == std::string::npos)
		return NULL;

	/* maybe this intializing code in the contructor */
	_reqVariables = reqVariables();
	_reqVariables.contentLength = -1;
	_reqVariables.clientFD = clientFD;
	_reqVariables.errorCode = 0;
	_reqVariables.errorMessage = "";
	_reqVariables.type = REQ_GET;

	_fullMessage = _buffer.substr(0, reqEnd + 4);
	_buffer.erase(0, reqEnd + 4);

	/* wouldn't this find the first "\r\n" of the CLRF? */
	size_t lineEnd = _fullMessage.find("\r\n");
	if (lineEnd == std::string::npos)
		return makeErrorRequest(400, "Bad request line", REQ_ERROR);

	std::string firstLine = _fullMessage.substr(0, lineEnd);
	if (errorReq = firstLineParse(firstLine))
		return errorReq;

	size_t current = lineEnd + 2;
	/* this while loops over all the headers */
	while (current < _fullMessage.size())
	{
		size_t next = _fullMessage.find("\r\n", current);
		if (next == std::string::npos)
			return makeErrorRequest(400, "Bad header line", REQ_ERROR);

		if (next == current)
			break;

		std::string headerLine = _fullMessage.substr(current, next - current);
		if (errorReq = headerParse(headerLine))
			return errorReq;

		current = next + 2;
	}

	reqVariables *currentVars = new reqVariables;
	*currentVars = _reqVariables;
	return new Request(currentVars);
}

Request* HttpParser::headerParse(std::string headerLine)
{
	size_t firstColon = headerLine.find(':');
	if (firstColon == std::string::npos)
		return makeErrorRequest(400, "Bad header line: missing colon", REQ_ERROR);

	HeaderField temp;
	temp.name = trimSpaces(headerLine.substr(0, firstColon));
	temp.value = trimSpaces(headerLine.substr(firstColon + 1));

	if (temp.name.empty())
		return makeErrorRequest(400, "Bad header line: empty header name", REQ_ERROR);

	_reqVariables.headers.push_back(temp);
	return NULL;
}

Request* HttpParser::firstLineParse(std::string firstLine)
{
	size_t firstSpace = firstLine.find(' ');
	if (firstSpace == std::string::npos)
		return makeErrorRequest(400, "Bad request line: missing first space", REQ_ERROR);

	size_t secondSpace = firstLine.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
		return makeErrorRequest(400, "Bad request line: missing second space", REQ_ERROR);

	size_t thirdSpace = firstLine.find(' ', secondSpace + 1);
	if (thirdSpace != std::string::npos)
		return makeErrorRequest(400, "Bad request line: too many spaces", REQ_ERROR);

	_reqVariables.method = firstLine.substr(0, firstSpace);
	_reqVariables.requestPath = firstLine.substr(firstSpace + 1,
		secondSpace - firstSpace - 1);
	_reqVariables.requestVersion = firstLine.substr(secondSpace + 1);

	if (_reqVariables.method.empty() || _reqVariables.requestPath.empty()
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
/* 
	std::cout << "method is: [" << _reqVariables.method << "]" << std::endl;
	std::cout << "Path is: [" << _reqVariables.requestPath << "]" << std::endl;
	std::cout << "Version is: [" << _reqVariables.requestVersion << "]" << std::endl; */

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
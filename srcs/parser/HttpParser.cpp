#include "HttpParser.hpp"

HttpParser::HttpParser()
{
	std::cout << "HttpParser constructed\n";
}

HttpParser::~HttpParser() {}

Request* HttpParser::makeErrorRequest(void)
{
	reqVariables *currentVars = new reqVariables;
	*currentVars = _reqVariables;
	return new Request(currentVars);
}

Request* HttpParser::parse(char *rawBuffer, size_t bitesRead, int clientFD)
{
	_buffer.append(rawBuffer, bitesRead);

	size_t reqEnd = _buffer.find("\r\n\r\n");
	if (reqEnd == std::string::npos)
		return NULL;

	_reqVariables = reqVariables();
	_reqVariables.contentLength = -1;
	_reqVariables.clientFD = clientFD;
	_reqVariables.errorCode = 0;
	_reqVariables.errorMessage = "";
	_reqVariables.type = REQ_GET;

	_fullMessage = _buffer.substr(0, reqEnd + 4);
	_buffer.erase(0, reqEnd + 4);

	size_t lineEnd = _fullMessage.find("\r\n");
	if (lineEnd == std::string::npos)
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad request line";
		_reqVariables.type = REQ_ERROR;
		return makeErrorRequest();
	}

	std::string firstLine = _fullMessage.substr(0, lineEnd);
	if (!firstLineParse(firstLine))
		return makeErrorRequest();

	size_t current = lineEnd + 2;

	while (current < _fullMessage.size())
	{
		size_t next = _fullMessage.find("\r\n", current);
		if (next == std::string::npos)
		{
			_reqVariables.errorCode = 400;
			_reqVariables.errorMessage = "Bad header line";
			_reqVariables.type = REQ_ERROR;
			return makeErrorRequest();
		}

		if (next == current)
			break;

		std::string headerLine = _fullMessage.substr(current, next - current);
		if (!headerParse(headerLine))
			return makeErrorRequest();

		current = next + 2;
	}

	reqVariables *currentVars = new reqVariables;
	*currentVars = _reqVariables;
	return new Request(currentVars);
}

bool HttpParser::headerParse(std::string headerLine)
{
	size_t firstColon = headerLine.find(':');
	if (firstColon == std::string::npos)
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad header line: missing colon";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	HeaderField temp;
	temp.name = trimSpaces(headerLine.substr(0, firstColon));
	temp.value = trimSpaces(headerLine.substr(firstColon + 1));

	if (temp.name.empty())
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad header line: empty header name";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	_reqVariables.headers.push_back(temp);
	return true;
}

bool HttpParser::firstLineParse(std::string firstLine)
{
	size_t firstSpace = firstLine.find(' ');
	if (firstSpace == std::string::npos)
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad request line: missing first space";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	size_t secondSpace = firstLine.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad request line: missing second space";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	size_t thirdSpace = firstLine.find(' ', secondSpace + 1);
	if (thirdSpace != std::string::npos)
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad request line: too many spaces";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	_reqVariables.method = firstLine.substr(0, firstSpace);
	_reqVariables.requestPath = firstLine.substr(firstSpace + 1,
		secondSpace - firstSpace - 1);
	_reqVariables.requestVersion = firstLine.substr(secondSpace + 1);

	if (_reqVariables.method.empty() || _reqVariables.requestPath.empty()
		|| _reqVariables.requestVersion.empty())
	{
		_reqVariables.errorCode = 400;
		_reqVariables.errorMessage = "Bad request line: empty component";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	if (_reqVariables.method == "GET")
		_reqVariables.type = REQ_GET;
	else if (_reqVariables.method == "POST")
		_reqVariables.type = REQ_POST;
	else if (_reqVariables.method == "DELETE")
		_reqVariables.type = REQ_DELETE;
	else
	{
		_reqVariables.errorCode = 405;
		_reqVariables.errorMessage = "Unsupported method";
		_reqVariables.type = REQ_ERROR;
		return false;
	}

	if (_reqVariables.requestVersion != "HTTP/1.1"
		&& _reqVariables.requestVersion != "HTTP/1.0")
	{
		_reqVariables.errorCode = 505;
		_reqVariables.errorMessage = "Unsupported HTTP version";
		_reqVariables.type = REQ_ERROR;
		return false;
	}
/* 
	std::cout << "method is: [" << _reqVariables.method << "]" << std::endl;
	std::cout << "Path is: [" << _reqVariables.requestPath << "]" << std::endl;
	std::cout << "Version is: [" << _reqVariables.requestVersion << "]" << std::endl; */

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
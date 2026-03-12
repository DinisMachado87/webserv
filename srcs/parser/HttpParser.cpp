#include "HttpParser.hpp"

// Constructors and destructors
HttpParser::HttpParser()
{
	std::cout << "HttpParser constructed\n";
}

HttpParser::~HttpParser() {}


// Methods
Request* HttpParser::parse(char *rawBuffer, size_t bitesRead, int clientFD)
{
	_buffer.append(rawBuffer, bitesRead);

	size_t reqEnd = _buffer.find("\r\n\r\n");
	if (reqEnd == std::string::npos)
		return NULL;

	_reqVariables = reqVariables();
	_reqVariables.contentLength = -1;
	_reqVariables.clientFD = clientFD;

	_fullMessage = _buffer.substr(0, reqEnd + 4);
	_buffer.erase(0, reqEnd + 4);

	size_t lineEnd = _fullMessage.find("\r\n");
	if (lineEnd == std::string::npos)
	{
		std::cout << "400: Bad request line\n";
		return NULL;
	}

	std::string firstLine = _fullMessage.substr(0, lineEnd);
	if (!firstLineParse(firstLine))
		return NULL;

	size_t headerStart = lineEnd + 2;

	// If there is no second line/header yet, accept request for now
	if (_fullMessage.substr(headerStart, 2) != "\r\n")
	{
		size_t endFirstHeader = _fullMessage.find("\r\n", headerStart);
		if (endFirstHeader == std::string::npos)
		{
			std::cout << "400: Bad header line\n";
			return NULL;
		}

		std::string headerLine = _fullMessage.substr(headerStart, endFirstHeader - headerStart);
		if (!headerParse(headerLine))
			return NULL;
	}

	return new Request(_reqVariables);
}

bool HttpParser::headerParse(std::string headerLine)
{
	size_t firstSemi = headerLine.find(':');
	if (firstSemi == std::string::npos)
	{
		std::cout << "400: Bad header line: missing colon\n";
		return false;
	}

	HeaderField temp;
	temp.name = trimSpaces(headerLine.substr(0, firstSemi));
	temp.value = trimSpaces(headerLine.substr(firstSemi + 1));

	if (temp.name.empty())
	{
		std::cout << "400: Bad header line: empty header name\n";
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
		std::cout << "400: Bad request line: missing first space\n";
		return false;
	}

	size_t secondSpace = firstLine.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
	{
		std::cout << "400: Bad request line: missing second space\n";
		return false;
	}

	size_t thirdSpace = firstLine.find(' ', secondSpace + 1);
	if (thirdSpace != std::string::npos)
	{
		std::cout << "400: Bad request line: too many spaces\n";
		return false;
	}

	_reqVariables.method = firstLine.substr(0, firstSpace);
	_reqVariables.requestPath = firstLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	_reqVariables.requestVersion = firstLine.substr(secondSpace + 1);

	if (_reqVariables.method.empty() || _reqVariables.requestPath.empty()
		|| _reqVariables.requestVersion.empty())
	{
		std::cout << "400: Bad request line: empty component\n";
		return false;
	}

	if (_reqVariables.method != "GET" && _reqVariables.method != "POST"
		&& _reqVariables.method != "DELETE")
	{
		std::cout << "405: Unsupported method\n";
		return false;
	}

	if (_reqVariables.requestVersion != "HTTP/1.1" && _reqVariables.requestVersion != "HTTP/1.0")
	{
		std::cout << "505: Unsupported HTTP version\n";
		return false;
	}

	std::cout << "method is: [" << _reqVariables.method << "]" << std::endl;
	std::cout << "Path is: [" << _reqVariables.requestPath << "]" << std::endl;
	std::cout << "Version is: [" << _reqVariables.requestVersion << "]" << std::endl;

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


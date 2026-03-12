#include "HttpParser.hpp"

// Constructors and destructors
HttpParser::HttpParser()
{
	std::cout << "HttpParser constructed\n";
}

HttpParser::~HttpParser() {}


// Methods
Request* HttpParser::parse(char *rawBuffer, size_t bitesRead)
{
	Request *result = NULL;
	_buffer.append(rawBuffer, bitesRead);

	size_t reqEnd = _buffer.find("\r\n\r\n");;
	if (reqEnd ==  std::string::npos)
		return NULL;
	_fullMessage = _buffer.substr(0, reqEnd + 4);
	_buffer.erase(0, reqEnd + 4);
	std::cout << _fullMessage << std::endl;

	/* here I will call the constructor of Request and assign it to result */
	return result;
}


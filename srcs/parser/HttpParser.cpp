#include "HttpParser.hpp"

// Constructors and destructors
HttpParser::HttpParser()
{
	std::cout << "HttpParser constructed\n";
}

HttpParser::~HttpParser() {}


// Methods
Request* HttpParser::parse(char *rawBuffer)
{
	Request *result = NULL;
	_buffer += rawBuffer;
	std::cout << _buffer << std::endl;

	return result;
}

size_t HttpParser::findCRLF(const std::string& s) const
{
	for (size_t i = 0; i + 1 < s.size(); ++i)
	{
		if (s[i] == '\r' && s[i + 1] == '\n')
			return i;
	}
	return std::string::npos;
}
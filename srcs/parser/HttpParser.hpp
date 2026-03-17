#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "StrView.hpp"
#include "webServ.hpp"
#include "../requests/Request.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include <iostream>

class HttpParser
{
private:
	std::string		_buffer;
	std::string		_fullMessage;
	reqVariables	_reqVariables;

private:
	//explicit disables
	HttpParser(const HttpParser& other);
	HttpParser& operator=(const HttpParser& other);

	//private methods
	Request*	firstLineParse(std::string firstLine);
	Request*	headerParse(std::string headerLine);
	std::string	trimSpaces(const std::string& s) const;
	Request*	makeErrorRequest(int Code, std::string Message, e_response_type Type);

public:
	// Constructors and destructors
	HttpParser();
	~HttpParser();

	//pulic methods
	Request* parse(char *rawBuffer, size_t bitesRead, int clientFD);
};

#endif
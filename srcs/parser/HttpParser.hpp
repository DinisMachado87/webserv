#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include "StrView.hpp"
#include "webServ.hpp"
#include "/home/akosloff/Projects/webserv/webserv/srcs/requests/Request.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include <iostream>

class HttpParser {


private:
	std::string _buffer;
	std::string _fullMessage;

public:
	// Constructors and destructors
	HttpParser();
	~HttpParser();


private:
	//explicit disables
	HttpParser(const HttpParser& other);
	HttpParser& operator=(const HttpParser& other);

	//private methods:



public:
	// Methods
	Request* parse(char *rawBuffer, size_t bitesRead);
};

#endif
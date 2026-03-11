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

public:
	// Constructors and destructors
	HttpParser();
	~HttpParser();


private:
	//explicit disables
	HttpParser(const HttpParser& other);
	HttpParser& operator=(const HttpParser& other);

	//private methods:
	size_t findCRLF(const std::string& s) const;


public:
	// Methods
	Request* parse(char *rawBuffer);
};

#endif
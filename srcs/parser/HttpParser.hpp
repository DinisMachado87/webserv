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
#include <sstream>
#include <cctype>

class Server;

class HttpParser
{
private:
	std::string		_buffer;
	std::string		_fullMessage;
	reqVariables	_reqVariables;
	const Server*	_server;

private:
	// explicit disables
	HttpParser(const HttpParser& other);
	HttpParser& operator=(const HttpParser& other);

	// private methods
	Request*	firstLineParse(const std::string& firstLine);
	Request*	headerParse(const std::string& headerLine);
	std::string	trimSpaces(const std::string& s) const;
	std::string	toLower(const std::string& s) const;
	bool		isDigits(const std::string& s) const;
	Request*	makeErrorRequest(int code, const std::string& message, e_request_type type);
	void		resetReqVariables(int clientFD);
	Request* 	parseRequestTarget(const std::string& target);
	bool 		isValidHostValue(const std::string& s) const;
	Request*	validateHeaders(void);
	Request* 	eraseHeaderAndReturn(Request* err, size_t headerEnd);
	Request*	clearBufferAndReturn(Request* err);
	bool 		isValidHeaderName(const std::string& s) const;

public:
	// Constructors and destructors
	HttpParser();
	~HttpParser();

	// public methods
	Request*	parse(char *rawBuffer, size_t bytesRead, int clientFD, const Server& server);
};

#endif
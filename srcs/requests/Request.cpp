#include "Request.hpp"
#include <sys/socket.h>
#include <sstream>

/* Basic constructor with defaults */
/* Request::Request(Location* loc) :
	_body(),
	_variables(),
	_location(loc),
	_clientFD(-1)
{
	_variables.method = "GET";
	_variables.contentLength = -1;
	_variables.requestPath.clear();
	_variables.CONTENT_TYPE.clear();
	_variables.QUERY_STRING.clear();
	_variables.REMOTE_ADDR.clear();
	_variables.REMOTE_HOST.clear();
} */

Request::Request(reqVariables vars) :
	_body(),
	_variables(vars),
	_location(NULL),
	_clientFD(vars.clientFD)
{
}

Request::~Request(void)
{
}



void Request::respond(std::string message)
{
	std::string body = "<html><body><h1>" + message + "</h1></body></html>";

	std::ostringstream oss;
	oss << body.size();
	std::string contentLength = oss.str();

	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + contentLength + "\r\n"
		"\r\n" +
		body;

	send(_clientFD, response.c_str(), response.size(), 0);
}

const reqVariables& Request::getVariables() const
{
	return _variables;
}

const std::string& Request::getBody() const
{
	return _body;
}

int Request::getClientFD() const
{
	return _clientFD;
}

Location* Request::getLocation() const
{
	return _location;
}
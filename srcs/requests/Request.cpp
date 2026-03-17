#include "Request.hpp"
#include <sys/socket.h>
#include <sstream>

Request::Request(reqVariables *vars) :
	vars(vars),
	_location(NULL)
{
}

Request::~Request(void)
{
	delete vars;
}

void Request::respond()
{
	std::string statusLine = "HTTP/1.1 200 OK\r\n";
	std::string message = "Hi from request";

	if (vars->type == REQ_ERROR)
	{
		std::ostringstream err;
		err << vars->errorCode << " " << vars->errorMessage;
		message = err.str();

		if (vars->errorCode == 400)
			statusLine = "HTTP/1.1 400 Bad Request\r\n";
		else if (vars->errorCode == 405)
			statusLine = "HTTP/1.1 405 Method Not Allowed\r\n";
		else if (vars->errorCode == 505)
			statusLine = "HTTP/1.1 505 HTTP Version Not Supported\r\n";
		else
			statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
	}

	std::string body = "<html><body><h1>" + message + "</h1></body></html>";

	std::ostringstream oss;
	oss << body.size();
	std::string contentLength = oss.str();

	std::string response =
		statusLine +
		"Content-Type: text/html\r\n"
		"Content-Length: " + contentLength + "\r\n"
		"Connection: close\r\n"
		"\r\n" +
		body;

	send(vars->clientFD, response.c_str(), response.size(), 0);
}

const reqVariables& Request::getVariables() const
{
	return *vars;
}

Location* Request::getLocation() const
{
	return _location;
}






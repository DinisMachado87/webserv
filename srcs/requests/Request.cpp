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
	switch (vars->type)
	{
		case REQ_GET:
			return handleGet();
		case REQ_POST:
			return handlePost();
		case REQ_DELETE:
			return handleDelete();
		case REQ_ERROR:
			return handleError();
		default:
			return sendSimpleErrorResponse(500, "Internal Server Error", "Unknown request type");
	}
}

void Request::sendResponse(const std::string& statusLine, const std::string& body, const std::string& contentType, const std::string& connectionHeader)
{
	std::ostringstream oss;
	oss << body.size();
	std::string contentLength = oss.str();

	std::string response =
		statusLine +
		"Content-Type: " + contentType + "\r\n" +
		"Content-Length: " + contentLength + "\r\n" +
		"Connection: " + connectionHeader + "\r\n" +
		"\r\n" +
		body;

	send(vars->clientFD, response.c_str(), response.size(), 0);
}

void Request::sendSimpleErrorResponse(int code, const std::string& reason, const std::string& message)
{
	std::ostringstream title;
	title << code << " " << reason;

	std::string body = "<html><body><h1>" + title.str() +
		"</h1><p>" + message + "</p></body></html>";

	sendResponse("HTTP/1.1 " + title.str() + "\r\n", body, "text/html", "close");
}


void Request::handleGet()
{
	sendResponse("HTTP/1.1 200 OK\r\n",
		"<html><body><h1>GET request received</h1></body></html>",
		"text/html",
		"close");
}

void Request::handlePost()
{
	sendResponse("HTTP/1.1 200 OK\r\n",
		"<html><body><h1>POST request received</h1></body></html>",
		"text/html",
		"close");
}

void Request::handleDelete()
{
	sendResponse("HTTP/1.1 200 OK\r\n",
		"<html><body><h1>DELETE request received</h1></body></html>",
		"text/html",
		"close");
}


void Request::handleError()
{
	std::string reason = getReasonPhrase(vars->errorCode);
	sendSimpleErrorResponse(vars->errorCode, reason, vars->errorMessage);
}

static std::string getReasonPhrase(int code)
{
	switch (code)
	{
		case 400: return "Bad Request";
		case 405: return "Method Not Allowed";
		case 411: return "Length Required";
		case 413: return "Content Too Large";
		case 431: return "Request Header Fields Too Large";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default:  return "Internal Server Error";
	}
}

const reqVariables& Request::getVariables() const
{
	return *vars;
}

Location* Request::getLocation() const
{
	return _location;
}






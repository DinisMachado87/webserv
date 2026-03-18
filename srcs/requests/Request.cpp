#include "Request.hpp"
#include "../srcs/Server/server.hpp"
#include <sys/socket.h>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <sstream>


Request::Request(reqVariables *vars, const Server* server) :
	vars(vars),
	_location(NULL),
	_server(server),
	_resolvedPath(),
	_isDirectory(false),
	_isRegularFile(false),
	_isCgi(false)
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
	if (!matchLocation())
		return handleError();

	if (!isMethodAllowed(Location::GET))
	{
		setError(405, "Method Not Allowed");
		return handleError();
	}

	if (!buildResolvedPath())
		return handleError();

	if (!inspectResolvedPath())
		return handleError();

	if (_isDirectory)
		return handleGetDirectory();

	if (_isCgi)
		return handleGetCgi();

	if (_isRegularFile)
		return handleGetFile();

	setError(404, "Not Found");
	handleError();
}



void Request::handleGetFile()
{
	std::ifstream file(_resolvedPath.c_str(), std::ios::in | std::ios::binary);
	std::ostringstream buffer;

	if (!file)
	{
		setError(403, "Failed to open file");
		return handleError();
	}

	buffer << file.rdbuf();
	sendResponse("HTTP/1.1 200 OK\r\n", buffer.str(), "text/html", "close");
}

void Request::handleGetCgi()
{
	setError(501, "CGI handling not implemented yet");
	handleError();
}

void Request::handleGetDirectory()
{
	setError(403, "Directory handling not implemented yet");
	handleError();
}

bool Request::matchLocation()
{
	size_t i;
	size_t bestLen = 0;
	Location* best = NULL;
	const std::string& path = vars->requestPath;

	if (_server == NULL)
	{
		setError(500, "Missing server");
		return false;
	}

	for (i = 0; i < _server->_locations.size(); i++)
	{
		Location& loc = _server->_locations[i];
		const char* locPathC = loc.getPath();
		std::string locPath;

		if (locPathC == NULL)
			continue;
		locPath = locPathC;
		if (locPath.empty())
			continue;
		if (path.compare(0, locPath.size(), locPath) != 0)
			continue;

		if (path.size() > locPath.size()
			&& locPath[locPath.size() - 1] != '/'
			&& path[locPath.size()] != '/')
			continue;

		if (locPath.size() > bestLen)
		{
			bestLen = locPath.size();
			best = &loc;
		}
	}

	_location = best;
	if (_location == NULL)
	{
		setError(404, "No matching location");
		return false;
	}
	return true;
}

bool Request::isMethodAllowed(uchar method) const
{
	if (_location == NULL)
		return false;
	return _location->isAllowedMethod(method) != 0;
}

bool Request::buildResolvedPath()
{
	const char* rootC = NULL;
	const char* locPathC = NULL;
	std::string root;
	std::string locPath;
	std::string suffix;

	if (_location == NULL || _server == NULL)
	{
		setError(500, "Path resolution failed");
		return false;
	}

	rootC = _location->_overrides.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		rootC = _server->_defaults.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
	{
		setError(500, "Missing root");
		return false;
	}

	locPathC = _location->getPath();
	if (locPathC == NULL || locPathC[0] == '\0')
	{
		setError(500, "Missing location path");
		return false;
	}

	root = rootC;
	locPath = locPathC;

	if (vars->requestPath.compare(0, locPath.size(), locPath) != 0)
	{
		setError(500, "Location path mismatch");
		return false;
	}

	suffix = vars->requestPath.substr(locPath.size());

	if (!root.empty() && root[root.size() - 1] == '/' &&
		!suffix.empty() && suffix[0] == '/')
		_resolvedPath = root + suffix.substr(1);
	else if ((!root.empty() && root[root.size() - 1] != '/') &&
			(suffix.empty() || suffix[0] != '/'))
		_resolvedPath = root + "/" + suffix;
	else
		_resolvedPath = root + suffix;

	if (_resolvedPath.empty())
	{
		setError(500, "Resolved path is empty");
		return false;
	}
	return true;
}



bool Request::inspectResolvedPath()
{
	struct stat st;

	if (stat(_resolvedPath.c_str(), &st) == -1)
	{
		setError(404, "Not Found");
		return false;
	}

	_isDirectory = S_ISDIR(st.st_mode);
	_isRegularFile = S_ISREG(st.st_mode);
	_isCgi = (_isRegularFile && isCgiPath());

	return true;
}

bool Request::isCgiPath() const
{
	size_t dot;
	std::string ext;
	const char* cgiExec;

	if (_location == NULL)
		return false;

	dot = _resolvedPath.rfind('.');
	if (dot == std::string::npos)
		return false;

	ext = _resolvedPath.substr(dot);
	cgiExec = _location->findCgiPath(ext.c_str());
	return (cgiExec != NULL);
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

void Request::setError(int code, const std::string& message)
{
	vars->type = REQ_ERROR;
	vars->errorCode = code;
	vars->errorMessage = message;
}

static std::string getReasonPhrase(int code)
{
	switch (code)
	{
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 411: return "Length Required";
		case 413: return "Content Too Large";
		case 431: return "Request Header Fields Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default:  return "Internal Server Error";
	}
}

const reqVariables& Request::getVariables() const
{
	return *vars;
}


const Location* Request::getLocation() const
{
	return _location;
}




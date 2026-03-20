/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/20 08:41:46 by akosloff          #+#    #+#             */
/*   Updated: 2026/03/20 13:48:16 by akosloff         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include "../server/Server.hpp"
#include "../responses/Response.hpp"
#include "../responses/GetResponse.hpp"
#include "../responses/CGIResponse.hpp"
#include <sys/socket.h>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <sstream>


Request::Request(reqVariables *vars, const Server* server) :
	vars(vars),
	_location(NULL),
	_server(server)
/* 	_resolvedPath(),
	_isDirectory(false),
	_isRegularFile(false),
	_isCgi(false) */
{
}

Request::~Request(void)
{
	delete vars;
}

Response* Request::validateAndCreateResponse()
{
	std::cout << "validate and create response\n";

	if (vars == NULL || _server == NULL)
	{
		return NULL;
	}

	if (vars->type == REQ_ERROR)
	{
		std::ostringstream oss;
		oss << "<html><body><h1>Error "
			<< vars->errorCode
			<< "</h1><p>"
			<< vars->errorMessage
			<< "</p></body></html>";
		vars->body = oss.str();
		return new GetResponse(const_cast<Location*>(_location), vars);
	}

	if (vars->type == REQ_GET)
	{
		if (!validateGet())
		{
			std::ostringstream oss;
			oss << "<html><body><h1>Error "
				<< vars->errorCode
				<< "</h1><p>"
				<< vars->errorMessage
				<< "</p></body></html>";
			vars->body = oss.str();
			return new GetResponse(const_cast<Location*>(_location), vars);
		}
		if (vars->isCgi)
			return new CGIResponse(const_cast<Location*>(_location), vars);
		return new GetResponse(const_cast<Location*>(_location), vars);
	}

	setError(500, "Only GET is wired to Response for now");
	std::ostringstream oss;
	oss << "<html><body><h1>Error "
		<< vars->errorCode
		<< "</h1><p>"
		<< vars->errorMessage
		<< "</p></body></html>";
	vars->body = oss.str();
	return new GetResponse(const_cast<Location*>(_location), vars);
}


bool Request::validateGet()
{
	if (!matchLocation())
		return false;
	if (!isMethodAllowed(Location::GET))
	{
		setError(405, "Method Not Allowed");
		return false;
	}
	if (!buildResolvedPath())
		return false;

	if (resolveCgiScript())
		return true;

	if (!inspectResolvedPath())
		return false;
	return true;
}

bool Request::validatePost()
{
	if (!matchLocation())
		return false;

	if (!isMethodAllowed(Location::POST))
	{
		setError(405, "Method Not Allowed");
		return false;
	}

	if (!buildResolvedPath())
		return false;

	if (resolveCgiScript())
		return true;

	if (!inspectResolvedPath())
		return false;

	return true;
}

bool Request::validateDelete()
{
	if (!matchLocation())
		return false;
	if (!isMethodAllowed(Location::DELETE))
	{
		setError(405, "Method Not Allowed");
		return false;
	}
	if (!buildResolvedPath())
		return false;
	if (!inspectResolvedPath())
		return false;
	/* DELETE-specific rules */
	if (vars->isDirectory)
	{
		setError(403, "Cannot delete directory");
		return false;
	}
	if (!vars->isRegularFile)
	{
		setError(404, "Not Found");
		return false;
	}

	return true;
}





//finds the best matching location block (longest prefix match) for the request path.=bool Request::matchLocation()
bool Request::matchLocation()
{
	size_t i;
	size_t bestLen = 0;
	const Location* best = NULL;
	//create path reference just for shorter code
	const std::string& path = vars->requestPath;

	if (_server == NULL)
	{
		setError(500, "Missing server");
		return false;
	}
	//looping over all the locations in the config and checks for each of them
	for (i = 0; i < _server->_locations.size(); i++)
	{
		const Location& loc = _server->_locations[i];
		const char* locPathC = loc.getPath();
		std::string locPath;

		if (locPathC == NULL)
			continue;
		locPath = locPathC;
		if (locPath.empty())
			continue;
		//checks if request starts with location
		if (path.compare(0, locPath.size(), locPath) != 0)
			continue;

		//boundary check so /img will not match /images 
		if (path.size() > locPath.size()
			&& locPath[locPath.size() - 1] != '/'
			&& path[locPath.size()] != '/')
			continue;

		//longest match wins - without this just the first / will match
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
	std::cout << "Matched location: " << _location->getPath() << std::endl;
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
	if (vars->requestPath.find("..") != std::string::npos)
	{
		setError(403, "Forbidden path");
		return false;
	}
	vars->resolvedPath = buildResolvedPathFromUrl(vars->requestPath);
	if (vars->resolvedPath.empty())
	{
		setError(500, "Path resolution failed");
		return false;
	}
	std::cout << "Resolved path: " << vars->resolvedPath << std::endl;
	return true;
}

std::string Request::buildResolvedPathFromUrl(const std::string& urlPath) const
{
	const char* rootC = NULL;
	const char* locPathC = NULL;
	std::string root;
	std::string locPath;
	std::string suffix;
	std::string result;

	if (_location == NULL || _server == NULL)
		return "";

	//get root directory and validate
	rootC = _location->_overrides.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		rootC = _server->_defaults.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		return "";

	locPathC = _location->getPath();
	if (locPathC == NULL || locPathC[0] == '\0')
		return "";

	root = rootC;
	locPath = locPathC;

	if (urlPath.compare(0, locPath.size(), locPath) != 0)
		return "";

	//extract suffix
	suffix = urlPath.substr(locPath.size());

	//first if avoids // second avoids missing /, else is defualt
	if (!root.empty() && root[root.size() - 1] == '/'
		&& !suffix.empty() && suffix[0] == '/')
		result = root + suffix.substr(1);
	else if (!root.empty() && root[root.size() - 1] != '/'
		&& (suffix.empty() || suffix[0] != '/'))
		result = root + "/" + suffix;
	else
		result = root + suffix;

	return result;
}

bool Request::resolveCgiScript()
{
	std::string candidateUrl;
	std::string candidateResolved;
	std::string ext;
	size_t slashPos;
	size_t dotPos;
	struct stat st;
	const char* cgiExec;

	if (_location == NULL || _server == NULL)
		return false;

	candidateUrl = vars->requestPath;

	while (!candidateUrl.empty())
	{
		candidateResolved = buildResolvedPathFromUrl(candidateUrl);
		if (!candidateResolved.empty()
			&& stat(candidateResolved.c_str(), &st) == 0
			&& S_ISREG(st.st_mode))
		{
			dotPos = candidateResolved.rfind('.');
			if (dotPos != std::string::npos)
			{
				ext = candidateResolved.substr(dotPos);
				cgiExec = _location->findCgiPath(ext.c_str());
				if (cgiExec != NULL)
				{
					vars->resolvedPath = candidateResolved;
					vars->isDirectory = false;
					vars->isRegularFile = true;
					vars->isCgi = true;
					vars->scriptName = candidateUrl;
					if (vars->requestPath.size() > candidateUrl.size())
						vars->pathInfo = vars->requestPath.substr(candidateUrl.size());
					else
						vars->pathInfo.clear();
					return true;
				}
			}
		}

		slashPos = candidateUrl.rfind('/');
		if (slashPos == std::string::npos || slashPos == 0)
			break;
		candidateUrl.erase(slashPos);
	}
	return false;
}

/* It takes the request path (e.g. /images/logo.png) and turns it into a real file path on disk (e.g. /home/akosloff/images/logo.png).
I will need to normalize, sanitize paths?
bool Request::buildResolvedPath()
{
	const char* rootC = NULL;
	const char* locPathC = NULL;
	std::string root;
	std::string locPath;
	std::string suffix;
	std::string originalPath;
	size_t cgiPos;
	size_t endOfScript;


	if (_location == NULL || _server == NULL)
	{
		setError(500, "Path resolution failed");
		return false;
	}
	// keep original URL path before overwriting requestPath with filesystem path
	originalPath = vars->requestPath;
	vars->scriptName.clear();
	vars->pathInfo.clear();
	vars->resolvedPath.clear();

	//get root directory and validate
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


	//extract suffix
	suffix = originalPath.substr(locPath.size());
	
	//first if avoids // second avoids missing /, else is defualt
	if (!root.empty() && root[root.size() - 1] == '/' && !suffix.empty() && suffix[0] == '/')
		vars->resolvedPath = root + suffix.substr(1);
	else if ((!root.empty() && root[root.size() - 1] != '/') && (suffix.empty() || suffix[0] != '/'))
		vars->resolvedPath = root + "/" + suffix;
	else
		vars->resolvedPath = root + suffix;

	// fill CGI-related variables from original URL path
	cgiPos = originalPath.find(".cgi");
	if (cgiPos != std::string::npos)
	{
		endOfScript = cgiPos + 4;
		vars->scriptName = originalPath.substr(0, endOfScript);
		if (endOfScript < originalPath.size())
			vars->pathInfo = originalPath.substr(endOfScript);
	}

	// temporary compatibility bridge: Response currently reads requestPath as file path
	vars->requestPath = vars->resolvedPath;
	std::cout << "Resolved path: " << vars->resolvedPath << std::endl;
	std::cout << "scriptName: " << vars->scriptName << std::endl;
	std::cout << "pathInfo: " << vars->pathInfo << std::endl;
	return true;
} */



bool Request::inspectResolvedPath()
{
	struct stat st;

	if (stat(vars->resolvedPath.c_str(), &st) == -1)
	{
		if (errno == EACCES)
			setError(403, "Forbidden");
		else
			setError(404, "Not Found");
		return false;
	}

	vars->isDirectory = S_ISDIR(st.st_mode);
	vars->isRegularFile = S_ISREG(st.st_mode);
	vars->isCgi = (vars->isRegularFile && isCgiPath());


	std::cout << "_isDirectory=" << vars->isDirectory << " _isRegularFile=" << vars->isRegularFile << " _isCgi=" << vars->isCgi << std::endl;

	return true;
}

bool Request::isCgiPath() const
{
	size_t dot;
	std::string ext;
	const char* cgiExec;

	if (_location == NULL)
		return false;

	dot = vars->resolvedPath.rfind('.');
	if (dot == std::string::npos)
		return false;

	ext = vars->resolvedPath.substr(dot);
	cgiExec = _location->findCgiPath(ext.c_str());
	return (cgiExec != NULL);
}



void Request::setError(int code, const std::string& message)
{
	vars->type = REQ_ERROR;
	vars->errorCode = code;
	vars->errorMessage = message;
}



const reqVariables& Request::getVariables() const
{
	return *vars;
}


const Location* Request::getLocation() const
{
	return _location;
}


int Request::getClientFD() const
{
	return vars->clientFD;
}


/* bool Request::validate()
{
	if (vars->type == REQ_ERROR)
		return false;
	if (vars->type == REQ_GET)
		return validateGet();
	if (vars->type == REQ_POST)
		return validatePost();
	if (vars->type == REQ_DELETE)
		return validateDelete();

	setError(500, "Unknown request type");
	return false;
} */


/* void Request::handlePost()
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
} */

/* 
void Request::sendSimpleErrorResponse(int code, const std::string& reason, const std::string& message)
{
	std::ostringstream title;
	title << code << " " << reason;

	std::string body = "<html><body><h1>" + title.str() +
		"</h1><p>" + message + "</p></body></html>";

	sendResponse("HTTP/1.1 " + title.str() + "\r\n", body, "text/html", "open");
}


void Request::handleGet()
{
	std::cout << "GET path: " << vars->requestPath << std::endl;

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
} */


/* void Request::sendResponse(const std::string& statusLine, const std::string& body, const std::string& contentType, const std::string& connectionHeader)
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
} */


/* std::string Request::getReasonPhrase(int code)
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
} */
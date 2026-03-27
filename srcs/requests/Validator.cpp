/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 00:00:00 by                   #+#    #+#             */
/*   Updated: 2026/03/26 16:08:44 by akosloff         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Validator.hpp"
#include "../server/Server.hpp"
#include "../responses/Response.hpp"
#include "../responses/GetResponse.hpp"
#include "../responses/CGIResponse.hpp"
#include "../responses/DirectoryResponse.hpp"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sys/stat.h>

Validator::Validator(const Server& server) :
	_server(server)
{
}

Validator::~Validator()
{
}

Response* Validator::handleRequest(Request& request)
{
	const Location* location = NULL;

	if (request.hasParseError())
		return makeErrorResponse(request.getParseErrorCode(),
			request.getParseErrorMessage());

	location = matchLocation(request);
	if (location == NULL)
		return makeErrorResponse(404, "No matching location");

	if (!isMethodAllowed(location, request))
		return makeMethodNotAllowedResponse(location);

	if (!validateRequest(request, location))
		return makeErrorResponse(400, "Bad Request");

	if (request.getType() == REQ_GET)
		return handleGet(request, location);
	if (request.getType() == REQ_POST)
		return handlePost(request, location);
	if (request.getType() == REQ_DELETE)
		return handleDelete(request, location);

	return makeErrorResponse(405, "Method Not Allowed");
}

Response* Validator::handleGet(Request& request, const Location* location)
{
	std::string	resolvedPath;
	std::string	scriptName;
	std::string	pathInfo;
	bool		isDirectory;
	bool		isRegularFile;

	isDirectory = false;
	isRegularFile = false;

	resolvedPath = buildResolvedPath(location, request);
	request.setFilePath(resolvedPath);
	if (resolvedPath.empty())
		return makeErrorResponse(500, "Path resolution failed");

	if (resolveCgiScript(location, request, scriptName, pathInfo, resolvedPath))
	{
		std::cout << "Resolved CGI script: " << resolvedPath << std::endl;
		std::cout << "scriptName=" << scriptName
			<< " pathInfo=" << pathInfo << std::endl;
		return new CGIResponse(const_cast<Location*>(location), &request, _server._listen[0].getPort());
	}

	if (!inspectResolvedPath(resolvedPath, isDirectory, isRegularFile))
	{
		if (errno == EACCES)
			return makeErrorResponse(403, "Forbidden");
		return makeErrorResponse(404, "Not Found");
	}

	std::cout << "Resolved path: " << resolvedPath << std::endl;
	std::cout << "_isDirectory=" << isDirectory
		<< " _isRegularFile=" << isRegularFile
		<< " _isCgi=" << isCgiPath(location, resolvedPath) << std::endl;

	if (isDirectory)
	{
		if (location->_overrides.isAutoindexed())
			return new DirectoryResponse(const_cast<Location*>(location), &request);
		else
			return makeErrorResponse(403, "Forbidden");
	}

	return new GetResponse(const_cast<Location*>(location), &request);
}

Response* Validator::handlePost(Request& request, const Location* location)
{
	std::string	resolvedPath;
	std::string	scriptName;
	std::string	pathInfo;
	bool		isDirectory;
	bool		isRegularFile;

	isDirectory = false;
	isRegularFile = false;

	resolvedPath = buildResolvedPath(location, request);
	request.setFilePath(resolvedPath);
	if (resolvedPath.empty())
		return makeErrorResponse(500, "Path resolution failed");

	if (resolveCgiScript(location, request, scriptName, pathInfo, resolvedPath))
	{
		std::cout << "Resolved CGI script: " << resolvedPath << std::endl;
		std::cout << "scriptName=" << scriptName
			<< " pathInfo=" << pathInfo << std::endl;
		return new CGIResponse(const_cast<Location*>(location), &request, _server._listen[0].getPort());
	}

	if (!inspectResolvedPath(resolvedPath, isDirectory, isRegularFile))
	{
		if (errno == EACCES)
			return makeErrorResponse(403, "Forbidden");
		return makeErrorResponse(404, "Not Found");
	}

	std::cout << "Resolved path: " << resolvedPath << std::endl;
	std::cout << "_isDirectory=" << isDirectory
		<< " _isRegularFile=" << isRegularFile
		<< " _isCgi=" << isCgiPath(location, resolvedPath) << std::endl;

	return new GetResponse(const_cast<Location*>(location), &request);
}

Response* Validator::handleDelete(Request& request, const Location* location)
{
	std::string	resolvedPath;
	bool		isDirectory;
	bool		isRegularFile;

	isDirectory = false;
	isRegularFile = false;

	resolvedPath = buildResolvedPath(location, request);
	request.setFilePath(resolvedPath);
	if (resolvedPath.empty())
		return makeErrorResponse(500, "Path resolution failed");

	if (!inspectResolvedPath(resolvedPath, isDirectory, isRegularFile))
	{
		if (errno == EACCES)
			return makeErrorResponse(403, "Forbidden");
		return makeErrorResponse(404, "Not Found");
	}

	if (isDirectory)
		return makeErrorResponse(403, "Cannot delete directory");
	if (!isRegularFile)
		return makeErrorResponse(404, "Not Found");

	std::cout << "Resolved path: " << resolvedPath << std::endl;
	std::cout << "_isDirectory=" << isDirectory
		<< " _isRegularFile=" << isRegularFile << std::endl;

	return new GetResponse(const_cast<Location*>(location), &request);
}

/*
** Finds the best matching location block (longest prefix match)
** for the request path.
*/
const Location* Validator::matchLocation(const Request& request) const
{
	size_t				i;
	size_t				bestLen;
	const Location*		best;
	const std::string&	path = request.getRequestPath();

	bestLen = 0;
	best = NULL;
	i = 0;
	while (i < _server._locations.size())
	{
		const Location&	loc = _server._locations[i];
		const char*		locPathC = loc.getPath();
		std::string		locPath;

		if (locPathC == NULL)
		{
			i++;
			continue;
		}
		locPath = locPathC;
		if (locPath.empty())
		{
			i++;
			continue;
		}
		if (path.compare(0, locPath.size(), locPath) != 0)
		{
			i++;
			continue;
		}
		if (path.size() > locPath.size()
			&& locPath[locPath.size() - 1] != '/'
			&& path[locPath.size()] != '/')
		{
			i++;
			continue;
		}
		if (locPath.size() > bestLen)
		{
			bestLen = locPath.size();
			best = &loc;
		}
		i++;
	}
	if (best != NULL)
		std::cout << "Matched location: " << best->getPath() << std::endl;
	return best;
}

bool Validator::isMethodAllowed(const Location* location, const Request& request) const
{
	if (location == NULL)
		return false;
	if (request.getType() == REQ_GET)
		return (location->isAllowedMethod(Location::GET) != 0);
	if (request.getType() == REQ_POST)
		return (location->isAllowedMethod(Location::POST) != 0);
	if (request.getType() == REQ_DELETE)
		return (location->isAllowedMethod(Location::DELETE) != 0);
	return false;
}

bool Validator::validateRequest(const Request& request, const Location* location) const
{
	(void)location;

	if (request.getRequestPath().find("..") != std::string::npos)
		return false;

	/*
	** Add extra validation here later if you want:
	** - body size against config
	** - upload constraints
	** - method-specific checks
	*/
	return true;
}

std::string Validator::buildResolvedPath(const Location* location, const Request& request) const
{
	const char*	rootC;
	const char*	locPathC;

	rootC = NULL;
	locPathC = NULL;
	if (location == NULL)
		return "";
	if (request.getRequestPath().find("..") != std::string::npos)
		return "";

	rootC = location->_overrides.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		rootC = _server._defaults.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		return "";

	locPathC = location->getPath();
	if (locPathC == NULL || locPathC[0] == '\0')
		return "";

	return buildResolvedPathFromUrl(rootC, locPathC, request.getRequestPath());
}

std::string Validator::buildResolvedPathFromUrl(const std::string& root, const std::string& locationPath, const std::string& requestPath) const
{
	std::string	suffix;
	std::string	result;

	if (root.empty() || locationPath.empty())
		return "";
	if (requestPath.compare(0, locationPath.size(), locationPath) != 0)
		return "";

	suffix = requestPath.substr(locationPath.size());

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

bool Validator::inspectResolvedPath(const std::string& resolvedPath, bool& isDirectory,	bool& isRegularFile) const
{
	struct stat	st;

	if (stat(resolvedPath.c_str(), &st) == -1) // stat fills the stat struct with info about the file at resolvedPath, or returns -1 on error
		return false;

	isDirectory = S_ISDIR(st.st_mode);
	isRegularFile = S_ISREG(st.st_mode);
	return true;
}

bool Validator::isCgiPath(const Location* location,
	const std::string& resolvedPath) const
{
	size_t		dot;
	std::string	ext;
	const char*	cgiExec;

	if (location == NULL)
		return false;

	dot = resolvedPath.rfind('.');
	if (dot == std::string::npos)
		return false;

	ext = resolvedPath.substr(dot);
	cgiExec = location->findCgiPath(ext.c_str());
	return (cgiExec != NULL);
}

bool Validator::resolveCgiScript(const Location* location,
	const Request& request,
	std::string& scriptName,
	std::string& pathInfo,
	std::string& resolvedPath) const
{
	std::string	candidateUrl;
	std::string	candidateResolved;
	std::string	ext;
	size_t		slashPos;
	size_t		dotPos;
	struct stat	st;
	const char*	rootC;
	const char*	locPathC;
	const char*	cgiExec;

	rootC = NULL;
	locPathC = NULL;
	cgiExec = NULL;
	if (location == NULL)
		return false;

	rootC = location->_overrides.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		rootC = _server._defaults.getRoot();
	if (rootC == NULL || rootC[0] == '\0')
		return false;

	locPathC = location->getPath();
	if (locPathC == NULL || locPathC[0] == '\0')
		return false;

	candidateUrl = request.getRequestPath();
	while (!candidateUrl.empty())
	{
		candidateResolved = buildResolvedPathFromUrl(rootC, locPathC, candidateUrl);
		if (!candidateResolved.empty()
			&& stat(candidateResolved.c_str(), &st) == 0
			&& S_ISREG(st.st_mode))
		{
			dotPos = candidateResolved.rfind('.');
			if (dotPos != std::string::npos)
			{
				ext = candidateResolved.substr(dotPos);
				cgiExec = location->findCgiPath(ext.c_str());
				if (cgiExec != NULL)
				{
					resolvedPath = candidateResolved;
					scriptName = candidateUrl;
					if (request.getRequestPath().size() > candidateUrl.size())
						pathInfo = request.getRequestPath().substr(candidateUrl.size());
					else
						pathInfo.clear();
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

Response* Validator::makeErrorResponse(int code, const std::string& message) const
{
	(void)code;
	(void)message;

	/*
	** Replace this with your real error response creation.
	** Example:
	** return new Response(...);
	*/
	return NULL;
}

Response* Validator::makeMethodNotAllowedResponse(const Location* location) const
{
	(void)location;

	/*
	** Later you may want to build an Allow header from location.
	*/
	return makeErrorResponse(405, "Method Not Allowed");
}
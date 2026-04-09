/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validator.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/23 15:59:17 by akosloff          #+#    #+#             */
/*   Updated: 2026/03/25 10:50:41 by akosloff         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include "../requests/Request.hpp"

class Server;
class Response;
struct Location;

class Validator
{
public:
	Validator(const Server& server);
	~Validator();

	Response* handleRequest(Request* request);

private:
	Validator();
	Validator(const Validator& other);
	Validator& operator=(const Validator& other);

private:
	const Server& _server;

private:
	Response* handleGet(Request* request, const Location* location);
	Response* handlePost(Request* request, const Location* location);
	Response* handleDelete(Request* request, const Location* location);
	Response* handleDirectory(Request* request, const Location* location, const std::string& directoryPath) const;

	const Location* matchLocation(const Request* request) const;
	bool isMethodAllowed(const Location* location, const Request* request) const;
	bool validateRequest(const Request* request, const Location* location) const;

	std::string buildResolvedPath(const Location* location, const Request* request) const;
	std::string buildResolvedPathFromUrl(const std::string& root, const std::string& locationPath, const std::string& requestPath) const;

	bool inspectResolvedPath(const std::string& resolvedPath, bool& isDirectory, bool& isRegularFile) const;

	bool isCgiPath(const Location* location, const std::string& resolvedPath) const;

	bool resolveCgiScript(const Location* location,	const Request* request,	std::string& scriptName, std::string& pathInfo,	std::string& resolvedPath) const;

	Response* makeErrorResponse(Request* request, const Location* location, int code, const std::string& message) const;
	Response* makeMethodNotAllowedResponse(Request* request, const Location* location) const;
};




/* Responsibility:

take Request + Server config
route request
validate against config
resolve path
decide what kind of processing is needed
create the correct Response

This is the layer that should contain things like:

location matching
allowed methods
body size checks
file existence checks
directory handling
CGI detection
autoindex decision
upload/delete logic

This is the right place for almost all of the logic that is currently in Request.

So the question Validator answers is:

“Given this Request and this server config, what response should be produced?” */

#include "Request.hpp"
#include "HttpRequest.hpp"

/* Basic constructor with defaults */
Request::Request(Location* loc) :
	_body(),
	_variables(),
	_location(loc),
	_clientFD(-1)
{
	_variables.method = GET;
	_variables.contentLength = -1;
	_variables.requestPath.clear();
	_variables.CONTENT_TYPE.clear();
	_variables.QUERY_STRING.clear();
	_variables.REMOTE_ADDR.clear();
	_variables.REMOTE_HOST.clear();
}

/* Constructor that builds Request directly from parsed HTTP data */
Request::Request(Location* loc,
				 const HttpRequest& parsed,
				 int clientFD,
				 const std::string& remoteAddr,
				 const std::string& remoteHost) :
	_body(parsed.body),
	_variables(),
	_location(loc),
	_clientFD(clientFD)
{
	if (parsed.method == METHOD_GET)
		_variables.method = GET;
	else if (parsed.method == METHOD_POST)
		_variables.method = POST;
	else
		_variables.method = DELETE;

	_variables.contentLength = parsed.contentLength;
	_variables.requestPath = parsed.path;
	_variables.CONTENT_TYPE = parsed.getHeader("Content-Type");
	_variables.QUERY_STRING = parsed.queryString;
	_variables.REMOTE_ADDR = remoteAddr;
	_variables.REMOTE_HOST = remoteHost;
}

Request::~Request(void)
{
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
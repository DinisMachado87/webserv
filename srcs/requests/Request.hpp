/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akosloff <akosloff@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/20 08:41:36 by akosloff          #+#    #+#             */
/*   Updated: 2026/04/08 11:39:23 by akosloff         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once


#include <cstddef>
#include <string>
#include <vector>

struct HeaderField
{
	std::string name;
	std::string value;
};

enum e_request_type
{
	REQ_GET,
	REQ_POST,
	REQ_DELETE,
	REQ_ERROR
};

class Request
{
public:
	Request();
	~Request();

	// setters used by HttpParser
	void setMethod(const std::string& method);
	void setType(e_request_type type);
	void setRequestTarget(const std::string& target);
	void setRequestPath(const std::string& path);
	void setQueryString(const std::string& query);
	void setRequestVersion(const std::string& version);
	void setHost(const std::string& host);
	void setContentType(const std::string& contentType);
	void setContentLength(size_t contentLength);
	void setHasContentLength(bool hasContentLength);
	void setTransferEncoding(const std::string& transferEncoding);
	void setChunkSize(size_t chunkSize);
	void setFilePath(const std::string& filePath);
	void setScriptName(const std::string& scriptName);
	void setPathInfo(const std::string& pathInfo);
	void setBody(const std::string& body);
	void setClientFD(int clientFD);
	void setRemoteAddr(const std::string& remoteAddr);
	void setRemoteHost(const std::string& remoteHost);
	void addHeader(const std::string& name, const std::string& value);

	// parse error state
	void setParseError(int code, const std::string& message);
	bool hasParseError() const;
	int getParseErrorCode() const;
	const std::string& getParseErrorMessage() const;

	// getters used by Vaidator
	const std::string& getMethod() const;
	e_request_type getType() const;
	const std::string& getRequestTarget() const;
	const std::string& getRequestPath() const;
	const std::string& getQueryString() const;
	const std::string& getRequestVersion() const;
	const std::string& getHost() const;
	const std::string& getContentType() const;
	size_t getContentLength() const;
	bool hasContentLength() const;
	const std::string& getTransferEncoding() const;
	size_t getChunkSize() const;
	const std::string& getFilePath() const;
	const std::string& getScriptName() const;
	const std::string& getPathInfo() const;
	const std::string& getBody() const;
	int getClientFD() const;
	const std::string& getRemoteAddr() const;
	const std::string& getRemoteHost() const;
	const std::vector<HeaderField>& getHeaders() const;

	bool isGet() const;
	bool isPost() const;
	bool isDelete() const;

	// helpers
	bool hasHeader(const std::string& name) const;
	std::string getHeaderValue(const std::string& name) const;
	void printRequest() const;
	static std::string sizeToString(size_t value);

private:
	Request(const Request& other);
	Request& operator=(const Request& other);

private:
	std::string				_method;
	e_request_type			_type;
	std::string				_requestTarget; // the raw target from the request line, before any parsing
	std::string				_requestPath; // the part before any ? in the target
	std::string				_queryString; // the part after ? in the target
	std::string				_requestVersion;
	std::string				_host;
	std::string				_scriptName; // the part of the resolved path that corresponds to the CGI script, set by Validator after resolution
	std::string				_pathInfo; // the part of the resolved path that corresponds to PATH_INFO, set by Validator after resolution
	std::string				_contentType;
	std::string				_filePath; // the resolved filesystem path, set by Validator after resolution
	std::string				_transferEncoding;
	size_t					_contentLength;
	bool					_hasContentLength;
	size_t					_chunkSize;
	std::string				_body;
	int						_clientFD;
	std::string				_remoteAddr;
	std::string				_remoteHost;
	std::vector<HeaderField> _headers;

	bool					_hasParseError;
	int						_parseErrorCode;
	std::string				_parseErrorMessage;
};


/* Responsibility:

represent one HTTP request
own parsed request data
expose getters
maybe do only request-internal validation

Examples:

method
target/path
query string
version
headers
body
host
content-length

It should not:

know filesystem paths
know server root resolution
know how to serve files
know how to execute CGI
know how to create a Response

So Request becomes a real data model object.

URL can split into 3 parts:
after ? is the query string
cgi path info will be everything before query
file path is everything before cgi or html


*/


#include "Request.hpp"

Request::Request() :
	_method(),
	_type(REQ_ERROR),
	_requestTarget(),
	_requestPath(),
	_queryString(),
	_requestVersion(),
	_host(),
	_scriptName(),
	_pathInfo(),
	_contentType(),
	_filePath(),
	_transferEncoding(),
	_contentLength(0),
	_hasContentLength(false),
	_chunkSize(0),
	_body(),
	_clientFD(-1),
	_remoteAddr(),
	_remoteHost(),
	_headers(),
	_hasParseError(false),
	_parseErrorCode(0),
	_parseErrorMessage()
{
}

Request::~Request()
{
}

/* ==================== Setters ==================== */

void Request::setMethod(const std::string& method)
{
	_method = method;
}

void Request::setType(e_request_type type)
{
	_type = type;
}

void Request::setRequestTarget(const std::string& target)
{
	_requestTarget = target;
}

void Request::setRequestPath(const std::string& path)
{
	_requestPath = path;
}

void Request::setQueryString(const std::string& query)
{
	_queryString = query;
}

void Request::setRequestVersion(const std::string& version)
{
	_requestVersion = version;
}

void Request::setHost(const std::string& host)
{
	_host = host;
}

void Request::setContentType(const std::string& contentType)
{
	_contentType = contentType;
}

void Request::setContentLength(size_t contentLength)
{
	_contentLength = contentLength;
}

void Request::setHasContentLength(bool hasContentLength)
{
	_hasContentLength = hasContentLength;
}

void Request::setTransferEncoding(const std::string& transferEncoding)
{
	_transferEncoding = transferEncoding;
}

void Request::setChunkSize(size_t chunkSize)
{
	_chunkSize = chunkSize;
}

void Request::setFilePath(const std::string& filePath)
{
	_filePath = filePath;
}

void Request::setScriptName(const std::string& scriptName)
{
	_scriptName = scriptName;
}

void Request::setPathInfo(const std::string& pathInfo)
{
	_pathInfo = pathInfo;
}

void Request::setBody(const std::string& body)
{
	_body = body;
}

void Request::setClientFD(int clientFD)
{
	_clientFD = clientFD;
}

void Request::setRemoteAddr(const std::string& remoteAddr)
{
	_remoteAddr = remoteAddr;
}

void Request::setRemoteHost(const std::string& remoteHost)
{
	_remoteHost = remoteHost;
}

void Request::addHeader(const std::string& name, const std::string& value)
{
	HeaderField field;
	field.name = name;
	field.value = value;
	_headers.push_back(field);
}

/* ==================== Parse error ==================== */

void Request::setParseError(int code, const std::string& message)
{
	_hasParseError = true;
	_parseErrorCode = code;
	_parseErrorMessage = message;
	_type = REQ_ERROR;
}

bool Request::hasParseError() const
{
	return _hasParseError;
}

int Request::getParseErrorCode() const
{
	return _parseErrorCode;
}

const std::string& Request::getParseErrorMessage() const
{
	return _parseErrorMessage;
}

/* ==================== Getters ==================== */

const std::string& Request::getMethod() const
{
	return _method;
}

e_request_type Request::getType() const
{
	return _type;
}

const std::string& Request::getRequestTarget() const
{
	return _requestTarget;
}

const std::string& Request::getRequestPath() const
{
	return _requestPath;
}

const std::string& Request::getQueryString() const
{
	return _queryString;
}

const std::string& Request::getRequestVersion() const
{
	return _requestVersion;
}

const std::string& Request::getHost() const
{
	return _host;
}

const std::string& Request::getContentType() const
{
	return _contentType;
}

size_t Request::getContentLength() const
{
	return _contentLength;
}

bool Request::hasContentLength() const
{
	return _hasContentLength;
}

const std::string& Request::getTransferEncoding() const
{
	return _transferEncoding;
}

size_t Request::getChunkSize() const
{
	return _chunkSize;
}

const std::string& Request::getFilePath() const
{
	return _filePath;
}

const std::string& Request::getScriptName() const
{
	return _scriptName;
}

const std::string& Request::getPathInfo() const
{
	return _pathInfo;
}

const std::string& Request::getBody() const
{
	return _body;
}

int Request::getClientFD() const
{
	return _clientFD;
}

const std::string& Request::getRemoteAddr() const
{
	return _remoteAddr;
}

const std::string& Request::getRemoteHost() const
{
	return _remoteHost;
}

const std::vector<HeaderField>& Request::getHeaders() const
{
	return _headers;
}

/* ==================== Helpers ==================== */

bool Request::hasHeader(const std::string& name) const
{
	std::vector<HeaderField>::const_iterator it = _headers.begin();
	std::vector<HeaderField>::const_iterator end = _headers.end();

	while (it != end)
	{
		if (it->name == name)
			return true;
		++it;
	}
	return false;
}

std::string Request::getHeaderValue(const std::string& name) const
{
	std::vector<HeaderField>::const_iterator it = _headers.begin();
	std::vector<HeaderField>::const_iterator end = _headers.end();

	while (it != end)
	{
		if (it->name == name)
			return it->value;
		++it;
	}
	return "";
}

bool Request::isGet() const
{
	return _type == REQ_GET;
}

bool Request::isPost() const
{
	return _type == REQ_POST;
}

bool Request::isDelete() const
{
	return _type == REQ_DELETE;
}
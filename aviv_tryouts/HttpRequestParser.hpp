#ifndef HTTPREQUESTPARSER_HPP
#define HTTPREQUESTPARSER_HPP

#include "HttpRequest.hpp"
#include <string>
#include <cstddef>

class HttpRequestParser
{
public:
/*A state machine is a program that processes input step by step and changes its behavior depending on its current state.*/
	enum State
	{
		STATE_REQUEST_LINE,
		STATE_HEADERS,
		STATE_BODY,
		STATE_DONE,
		STATE_ERROR
	};

	enum Result
	{
		NEED_MORE,
		PARSE_DONE,
		PARSE_ERROR
	};

private:
	State		_state;
	HttpRequest	_request;
	int			_errorCode;
	std::string	_errorMessage;
	size_t		_contentLength;
	size_t		_maxBodySize;

public:
	HttpRequestParser();
	HttpRequestParser(const HttpRequestParser& other);
	HttpRequestParser& operator=(const HttpRequestParser& other);
	~HttpRequestParser();

	Result				feed(std::string& buffer);
	void				reset();

	const HttpRequest&	getRequest() const;
	int					getErrorCode() const;
	const std::string&	getErrorMessage() const;
	State				getState() const;

	void				setMaxBodySize(size_t maxBodySize);

private:
	Result	parseRequestLine(std::string& buffer);
	Result	parseHeaders(std::string& buffer);
	Result	parseBody(std::string& buffer);

	bool	parseContentLength(const std::string& value, size_t& out) const;
	size_t	findCRLF(const std::string& s) const;
	size_t	findChar(const std::string& s, char c, size_t start, size_t end) const;

	std::string		trim(const std::string& s) const;
	RequestMethod	parseMethod(const std::string& method) const;
	bool			isSupportedVersion(const std::string& version) const;
	void			splitTarget();

	void			setError(int code, const std::string& message);
};

#endif
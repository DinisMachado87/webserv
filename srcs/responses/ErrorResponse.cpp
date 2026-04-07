/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponse.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/27 14:06:18 by smoon             #+#    #+#             */
/*   Updated: 2026/04/07 14:17:40 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ErrorResponse.hpp"


ErrorResponse::ErrorResponse(Location* loc, Request* req) : Response(loc, req)
{
	_errorCode = this->_request->getParseErrorCode();
}
ErrorResponse::~ErrorResponse(void)
{

}

int	ErrorResponse::generateHeader(void)
{
	std::map<uint, const char*>::const_iterator	match;
	match = _errorTitles.find(_errorCode);
	const char* errorTitle = match->second;
	std::stringstream header;
	header << "HTTP/1.1 " << _errorCode << " " << errorTitle << "\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: " << SERVER_NAME << "\r\n";
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

int	ErrorResponse::setResponseBody(void)
{
	std::map<uint, const char*>::const_iterator	match;
	match = this->_errorTitles.find(_errorCode);
	if (match == _errorTitles.end())
	{
		_errorCode = 500;
		match = this->_errorTitles.find(_errorCode);
	}
	const char* path = this->_location->_overrides.findErrorFile(_errorCode);
	if (path)
	{
		int		fd = open(this->_request->getFilePath().c_str(), O_RDONLY);
		ssize_t	res = 1;
		ssize_t	chunk = 8192;
		ssize_t	size;
		ssize_t	oldSize = 0;
		this->_responseBody.resize(chunk);
		while (res > 0)
		{
			size = this->_responseBody.size();
			res = read(fd, &this->_responseBody[size - chunk], chunk);
			if (res < chunk)
				break ;
			oldSize = size;
			this->_responseBody.resize(size + chunk);
		}
		if (res != -1)
			this->_responseBody.resize(oldSize + res);
		return (0);
	}
	std::stringstream	body;
	const char* errorTitle = match->second;
	match = this->_errorBodies.find(_errorCode);
	const char* errorBody = match->second;
	body << _msg1 << _errorCode << " " << errorTitle;
	body << _msg2 << _errorCode << " " << errorTitle;
	body << _msg3 << errorBody;
	body << _msg4;
	_responseBody = body.str();
	return (0);
}

void	ErrorResponse::setErrorCode(uint code)
{
	_errorCode = code;
}

bool	Response::sendResponse(const int &clientFD)
{
	try {
		setResponseBody();
		generateHeader();
		ssize_t	ret = 0;
		ret = send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		ret = send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		throw std::runtime_error("ErrorResponse: failed to send ErrorResponse");
	}
	return 0;
}

std::map<uint, const char*>	makeTitles(void)
{
	std::map<uint, const char*>	errorTitles;
	errorTitles[400] = "Bad Request";
	errorTitles[401] = "Unauthorized";
	errorTitles[402] = "Payment Required";
	errorTitles[403] = "Forbidden";
	errorTitles[404] = "Not Found";
	errorTitles[405] = "Method Not Allowed";
	errorTitles[406] = "Not Acceptable";
	errorTitles[407] = "Proxy Authentication Required";
	errorTitles[408] = "Request Timeout";
	errorTitles[409] = "Conflict";
	errorTitles[410] = "Gone";
	errorTitles[411] = "Length Required";
	errorTitles[412] = "Precondition Failed";
	errorTitles[413] = "Content Too Large";
	errorTitles[414] = "URI Too Long";
	errorTitles[415] = "Unsupported Media Type";
	errorTitles[416] = "Range Not Satisfiable";
	errorTitles[417] = "Expectation Failed";
	errorTitles[418] = "I'm a teapot";
	errorTitles[421] = "Misdirected Request";
	errorTitles[422] = "Unprocessable Content";
	errorTitles[423] = "Locked";
	errorTitles[424] = "Failed Dependency";
	errorTitles[425] = "Too Early";
	errorTitles[426] = "Upgrade Required";
	errorTitles[428] = "Precondition Required";
	errorTitles[429] = "Too Many Requests";
	errorTitles[431] = "Request Header Fields Too Large";
	errorTitles[451] = "Unavailable For Legal Reasons";
	errorTitles[500] = "Internal Server Error";
	errorTitles[501] = "Not Implemented";
	errorTitles[502] = "Bad Gateway";
	errorTitles[503] = "Service Unavailable";
	errorTitles[504] = "Gateway Timeout";
	errorTitles[505] = "HTTP Version Not Supported";
	errorTitles[506] = "Variant Also Negotiates";
	errorTitles[507] = "Insufficient Storage";
	errorTitles[508] = "Loop Detected";
	errorTitles[510] = "Not Extended";
	errorTitles[511] = "Network Authentication Required";
	return errorTitles;
}

std::map<uint, const char*>	makeBodies(void)
{
	std::map<uint, const char*>	errorBodies;
	errorBodies[400] = "The request could not be understood by the server. Please check your input and try again.";
	errorBodies[401] = "You must be logged in to access this resource. Please sign in and try again.";
	errorBodies[402] = "Payment is required to access this resource. Please verify billing details and try again.";
	errorBodies[403] = "You do not have permission to view this page. Please contact support if you believe this is a mistake.";
	errorBodies[404] = "The requested page could not be found. Please check the address and try again.";
	errorBodies[405] = "This request method is not allowed for this resource. Please use a supported method.";
	errorBodies[406] = "The server cannot provide a response matching the requested format. Please adjust your request headers.";
	errorBodies[407] = "Proxy authentication is required. Please authenticate with your proxy and try again.";
	errorBodies[408] = "The request took too long and timed out. Please retry in a moment.";
	errorBodies[409] = "The request could not be completed due to a conflict. Please refresh and try again.";
	errorBodies[410] = "This resource is no longer available. Please check for an updated link.";
	errorBodies[411] = "A required Content-Length header is missing. Please include it and try again.";
	errorBodies[412] = "One or more preconditions in your request were not met. Please review and retry.";
	errorBodies[413] = "The uploaded content is too large. Please reduce the file size and try again.";
	errorBodies[414] = "The requested URI is too long. Please shorten the URL and try again.";
	errorBodies[415] = "The media type of the request is not supported. Please use a supported content type.";
	errorBodies[416] = "The requested byte range cannot be served. Please verify the requested range.";
	errorBodies[417] = "The server could not meet the request's Expect header requirements. Please remove or correct it.";
	errorBodies[418] = "The server refuses to brew coffee with a teapot. Please send a valid request.";
	errorBodies[421] = "The request was sent to the wrong server. Please retry with the correct host.";
	errorBodies[422] = "The request was well-formed but contains invalid data. Please correct the input and try again.";
	errorBodies[423] = "The requested resource is locked. Please try again later or contact support.";
	errorBodies[424] = "The request failed because a dependent action failed first. Please resolve the dependency and retry.";
	errorBodies[425] = "The server is unwilling to process this request yet. Please retry shortly.";
	errorBodies[426] = "A different protocol is required. Please upgrade and try again.";
	errorBodies[428] = "This request must be conditional to prevent conflicts. Please include the required conditional headers.";
	errorBodies[429] = "Too many requests were sent in a short time. Please wait and try again later.";
	errorBodies[431] = "Request headers are too large. Please reduce header size and retry.";
	errorBodies[451] = "This resource is unavailable for legal reasons. Please contact support for more information.";
	errorBodies[500] = "The server encountered an unexpected error. Please try again later.";
	errorBodies[501] = "The server does not support this request feature. Please use a different operation.";
	errorBodies[502] = "The server received an invalid response from an upstream service. Please try again soon.";
	errorBodies[503] = "The service is temporarily unavailable. Please retry in a few minutes.";
	errorBodies[504] = "An upstream service took too long to respond. Please try again later.";
	errorBodies[505] = "This HTTP version is not supported. Please use a supported HTTP version.";
	errorBodies[506] = "A server configuration error occurred during content negotiation. Please try again later.";
	errorBodies[507] = "The server does not have enough storage to complete the request. Please try again later.";
	errorBodies[508] = "The server detected a processing loop and stopped the request. Please contact support.";
	errorBodies[510] = "Additional request extensions are required. Please review the request and try again.";
	errorBodies[511] = "Network authentication is required. Please authenticate to the network and retry.";
	return errorBodies;
}

const std::map<uint, const char*>	ErrorResponse::_errorTitles = makeTitles();
const std::map<uint, const char*>	ErrorResponse::_errorBodies = makeBodies();
const char	ErrorResponse::_msg1[152] = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"UTF-8\" />\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n  <title>";
const char	ErrorResponse::_msg2[911] = "</title>\n  <style>\n    body {\n      margin: 0;\n      font-family: Arial, sans-serif;\n      background: #f7f7f7;\n      color: #222;\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      min-height: 100vh;\n    }\n    .card {\n      background: #fff;\n      border: 1px solid #ddd;\n      border-radius: 8px;\n      padding: 24px;\n      max-width: 460px;\n      text-align: center;\n      box-shadow: 0 2px 8px rgba(0,0,0,0.08);\n    }\n    h1 {\n      margin: 0 0 10px;\n      font-size: 28px;\n    }\n    p {\n      margin: 8px 0;\n      line-height: 1.5;\n    }\n    a {\n      color: #005fcc;\n      text-decoration: none;\n      font-weight: bold;\n    }\n    a:hover {\n      text-decoration: underline;\n    }\n    .small {\n      margin-top: 14px;\n      font-size: 13px;\n      color: #666;\n    }\n  </style>\n</head>\n<body>\n  <main class=\"card\">\n    <h1>";
const char	ErrorResponse::_msg3[14] = "</h1>\n    <p>";
const char	ErrorResponse::_msg4[27] = "</main>\n</body>\n</html>\n";


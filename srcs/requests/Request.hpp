#pragma once

#include <string>
#include <vector>


struct Location;
class HttpRequest;

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

class Server;

							//RFC2616
typedef	struct	reqVariables {		//{defaults}
std::string				method;			//GET / POST / PUT / DELETE etc
bool 					hasContentLength;
bool 					clearBufferOnError; 
size_t					contentLength;	//length of message body - taken from header or manually calculated if chunked {-1}
int						clientFD;
int						errorCode; //400 or 405 or etc
std::string				errorMessage; //actual message to print
std::string				requestPath;	//e.g. URL=example.com/cgi-bin/hello.cgi/user/admin {NULL} (parser needs to check for cgi-bin)
std::string 			contentType; //media type of message body - from header {NULL}
std::string 			queryString; //information for the CGI script to affect the return value - URL after '?' {NULL} e.g. URL=example.com/cgi-bin/hello.cgi/user/admin?query=date QUERY_STRING=query=date
std::string 			remoteAddr;	//network address of client sending the request (ipv4 or ipv6) {NULL}
std::string 			remoteHost; //domain name of the client sending the request, or {NULL}
std::string				requestVersion; //http/1.1 or other
std::string 			body;
std::string 			host;
std::string 			queryString;
size_t     				maxBodySize;
std::vector<HeaderField> headers; //the headers of the http request
e_request_type			type;	
}	reqVariables;

class	Request
{
public:
	Request(reqVariables *vars, const Server* server);
	~Request(void);

	void respond();
	const reqVariables&	getVariables() const;
	const std::string&	getBody() const;
	int					getClientFD() const;
	const Location*		getLocation() const;

private:
	reqVariables	*vars;
	const Location*	_location;
	const Server*	_server;
	std::string		_resolvedPath;
	bool			_isDirectory;	
	bool			_isRegularFile;
	bool			_isCgi;

private:
	Request(void);
	Request(const Request &other);
	Request &	operator=(const Request &other);

	void handleGet();
	void handlePost();
	void handleDelete();
	void handleError();
	bool validate()
	bool validateGet();
	bool validatePost();
{

	void sendResponse(const std::string& statusLine, const std::string& body, const std::string& contentType, const std::string& connectionHeader);
	void sendSimpleErrorResponse(int code, const std::string& reason, const std::string& message);

	//helper functions for handleGet()
	bool matchLocation();
	bool isMethodAllowed(unsigned char method) const;
	bool buildResolvedPath();
	bool inspectResolvedPath();
	bool isCgiPath() const;

	void handleGetFile();
	void handleGetDirectory();
	void handleGetCgi();

	std::string getReasonPhrase(int code);
	void	setError(int code, const std::string& message);

};



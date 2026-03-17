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



							//RFC2616
typedef	struct	reqVariables {		//{defaults}
std::string				method;			//GET / POST / PUT / DELETE etc
bool hasContentLength;
long					contentLength;	//length of message body - taken from header or manually calculated if chunked {-1}
int						clientFD;
int						errorCode; //400 or 405 or etc
std::string				errorMessage; //actual message to print
std::string				requestPath;	//e.g. URL=example.com/cgi-bin/hello.cgi/user/admin {NULL} (parser needs to check for cgi-bin)
std::string				CONTENT_TYPE;	//media type of message body - from header {NULL}
std::string				QUERY_STRING;	//information for the CGI script to affect the return value - URL after '?' {NULL} e.g. URL=example.com/cgi-bin/hello.cgi/user/admin?query=date QUERY_STRING=query=date
std::string				REMOTE_ADDR;	//network address of client sending the request (ipv4 or ipv6) {NULL}
std::string				REMOTE_HOST;	//domain name of the client sending the request, or {NULL}
std::string				requestVersion; //http/1.1 or other
std::string 			body;
std::vector<HeaderField> headers; //the headers of the http request
e_request_type			type;	
}	reqVariables;

class	Request
{
public:
	Request(reqVariables *vars);
/* 	Request(Location* loc, const HttpRequest& parsed, int clientFD,
		const std::string& remoteAddr, const std::string& remoteHost); */
	~Request(void);

	void respond(std::string message);
	const reqVariables&	getVariables() const;
	const std::string&	getBody() const;
	int					getClientFD() const;
	Location*			getLocation() const;

protected:
	reqVariables *vars;
	Location*	_location;

private:
	Request(void);
	Request(const Request &other);
	Request &	operator=(const Request &other);
};



#pragma once

#include <string>


struct Location;

enum requestMethod : unsigned char
{
	GET,
	POST,
	DELETE,
};

							//RFC2616
typedef	struct	reqVariables {		//{defaults}
requestMethod	method;			//GET / POST / PUT / DELETE etc
int				contentLength;	//length of message body - taken from header or manually calculated if chunked {-1}
std::string		requestPath;	//e.g. URL=example.com/cgi-bin/hello.cgi/user/admin {NULL} (parser needs to check for cgi-bin)
//these below might change to std::string
char*			CONTENT_TYPE;	//media type of message body - from header {NULL}
char*			QUERY_STRING;	//information for the CGI script to affect the return value - URL after '?' {NULL} e.g. URL=example.com/cgi-bin/hello.cgi/user/admin?query=date QUERY_STRING=query=date
char*			REMOTE_ADDR;	//network address of client sending the request (ipv4 or ipv6) {NULL}
char*			REMOTE_HOST;	//domain name of the client sending the request, or {NULL}
}	reqVariables;


class	Request
{
public:
	Request(Location* loc);
	~Request(void);


protected:
	std::string	_body;
	reqVariables _variables;
	Location*	_location;
	int			_clientFD;


private:
	Request(void);
	Request(const Request &other);
	Request &	operator=(const Request &other);
};

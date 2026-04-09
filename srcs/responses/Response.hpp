#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../requests/Request.hpp"
#include "../server/Server.hpp"

#define SERVER_NAME "nailedIt 1.0"

class	Response
{
public:
	Response(Location* loc, Request* req);
	virtual 		~Response(void);
	virtual bool	readBodyFirst(char buffer[], ssize_t bytesRead);
	virtual bool	readBodyLoop(char buffer[], ssize_t bytesRead);
	virtual bool	sendResponse(const int &clientFD);


protected:
	Location*		_location;
	std::string		_responseHeader;
	std::string		_responseBody;
	Request*		_request;
	virtual int		setResponseBody() = 0;
	virtual int		generateHeader(void) = 0;
	void			getTime(char* buf, int bufSize);

private:
	Response(void);
	Response(const Response &other);
	Response &	operator=(const Response &other);
};

void	initialise_everything(Location* loc, Request* req, Overrides* over);

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


class	Response
{
public:
	Response(Location* loc, reqVariables* vars, std::string* requestBody);
	virtual ~Response(void);


protected:
	Location*		_location;
	reqVariables*	_requestVars;
	std::string*	_requestBody;
	std::string		_responseHeader;
	std::string		_responseBody;
	void			getTime(char* buf, int bufSize);
	virtual int		sendResponse(const int& clientFD);
	// virtual int		generateResponse(void);

private:
	Response(void);
	Response(const Response &other);
	Response &	operator=(const Response &other);
};

void	initialise_everything(Location* loc, reqVariables* vars, Overrides* over, std::string* body);

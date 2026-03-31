

#pragma once

#include "Response.hpp"

class	DirectoryResponse : public Response
{
public:
	DirectoryResponse(Location* loc, Request* req);
	~DirectoryResponse(void);
	bool sendResponse(const int &clientFD);



	private:
	int	generateHeader(void);
	int	setResponseBody(void);


	DirectoryResponse(const DirectoryResponse &other);
	DirectoryResponse &	operator=(const DirectoryResponse &other);
};



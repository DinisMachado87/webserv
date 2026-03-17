#ifndef CLIENT_HPP
#define CLIENT_HPP


#include <poll.h>
#include <vector>
#include <cstddef>
#include <string>
#include "HttpRequestParser.hpp"

struct Client
{
	int                 fd;
	std::string         in;
	std::string         out;
	size_t              outSent;
	bool                shouldClose;
	bool                keepAlive;
	std::string         remoteAddr;
	std::string         remoteHost;
	HttpRequestParser   parser;

	Client();
	Client(int f, const std::string& addr, const std::string& host);
};

std::string to_lower(const std::string& s);
bool request_wants_keep_alive(const HttpRequest& req);

void queue_response(std::vector<pollfd>& fds,
                    std::vector<Client>& clients,
                    size_t idx,
                    const char* statusLine,
                    const char* body,
                    bool keepAlive);

#endif
#include "Client.hpp"
#include "Server.hpp"

#include <poll.h>
#include <vector>
#include <cstdio>
#include <cstring>

Client::Client() :
	fd(-1),
	in(),
	out(),
	outSent(0),
	shouldClose(false),
	keepAlive(false),
	remoteAddr(),
	remoteHost(),
	parser()
{
}

Client::Client(int f, const std::string& addr, const std::string& host) :
	fd(f),
	in(),
	out(),
	outSent(0),
	shouldClose(false),
	keepAlive(false),
	remoteAddr(addr),
	remoteHost(host),
	parser()
{
	parser.setMaxBodySize(1024 * 1024);
}

std::string to_lower(const std::string& s)
{
	std::string out = s;
	for (size_t i = 0; i < out.size(); ++i)
	{
		if (out[i] >= 'A' && out[i] <= 'Z')
			out[i] = static_cast<char>(out[i] - 'A' + 'a');
	}
	return out;
}

bool request_wants_keep_alive(const HttpRequest& req)
{
	std::string connection = to_lower(req.getHeader("Connection"));

	if (req.version == "HTTP/1.1")
		return connection != "close";

	if (req.version == "HTTP/1.0")
		return connection == "keep-alive";

	return false;
}

void queue_response(std::vector<pollfd>& fds,
                    std::vector<Client>& clients,
                    size_t idx,
                    const char* statusLine,
                    const char* body,
                    bool keepAlive)
{
	Client& c = clients[idx - 1];

	const char* connectionHeader = keepAlive ? "keep-alive" : "close";

	char header[512];
	std::snprintf(header, sizeof(header),
		"%s\r\n"
		"Content-Type: text/html; charset=utf-8\r\n"
		"Content-Length: %lu\r\n"
		"Connection: %s\r\n"
		"\r\n",
		statusLine,
		(unsigned long)std::strlen(body),
		connectionHeader);

	c.out.clear();
	c.out.append(header);
	c.out.append(body);
	c.outSent = 0;
	c.keepAlive = keepAlive;
	c.shouldClose = !keepAlive;

	set_client_events(fds, idx, POLLOUT);
}
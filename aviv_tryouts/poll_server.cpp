/* 
parser initated inside handleIn()
returns a request class (it can be either a healthy response or an error respone)

inside engine class there is Engine::pollLoop() this loops between the sockets

when Asocket class, it has _currentParser, created in the constructor.
each socket has its own ParserHttp instance.

inside Engine::pollLoop() we call connection class function handleIn()

inside handleIn()
we call ParserHttp.parse() method that takes the current buffer and the server.
ParserHttp will have to collect the buffers until CLRF, then return a pointer to a Request class (or derived class)
if CLRF doesn't exist yet we return NULL. but ParserHttp class has to keep the current state of buffer for next read.

 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include "HttpRequestParser.hpp"
#include "Request.hpp"

struct Client
{
	int					fd;
	std::string			in;
	std::string			out;
	size_t				outSent;
	bool				shouldClose;
	bool				keepAlive;
	std::string			remoteAddr;
	std::string			remoteHost;
	HttpRequestParser	parser;

	Client() :
		fd(-1),
		in(),
		out(),
		outSent(0),
		shouldClose(false),
		keepAlive(false),
		remoteAddr(),
		remoteHost(),
		parser() {}

	Client(int f, const std::string& addr, const std::string& host) :
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
};

static int set_reuseaddr(int fd)
{
	int yes = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
					 reinterpret_cast<const void*>(&yes), sizeof(yes)) < 0)
		return -1;
	return 0;
}

static int make_listen_socket(int port)
{
	int s = ::socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;

	if (set_reuseaddr(s) < 0)
		perror("setsockopt");

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
	{
		::close(s);
		return -1;
	}

	if (::listen(s, SOMAXCONN) < 0)
	{
		::close(s);
		return -1;
	}

	return s;
}

static void close_client(std::vector<pollfd>& fds,
						 std::vector<Client>& clients,
						 size_t idx)
{
	int fd = fds[idx].fd;
	::close(fd);

	fds.erase(fds.begin() + idx);
	clients.erase(clients.begin() + (idx - 1));
}

static void set_client_events(std::vector<pollfd>& fds, size_t idx, short events)
{
	fds[idx].events = events;
}

static std::string to_lower(const std::string& s)
{
	std::string out = s;
	for (size_t i = 0; i < out.size(); ++i)
	{
		if (out[i] >= 'A' && out[i] <= 'Z')
			out[i] = static_cast<char>(out[i] - 'A' + 'a');
	}
	return out;
}

static bool request_wants_keep_alive(const HttpRequest& req)
{
	std::string connection = to_lower(req.getHeader("Connection"));

	if (req.version == "HTTP/1.1")
		return connection != "close";

	if (req.version == "HTTP/1.0")
		return connection == "keep-alive";

	return false;
}

static void queue_response(std::vector<pollfd>& fds,
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

static void accept_client(std::vector<pollfd>& fds,
						  std::vector<Client>& clients)
{
	int listen_fd = fds[0].fd;

	sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	std::memset(&clientAddr, 0, sizeof(clientAddr));

	int cfd = ::accept(listen_fd,
					   reinterpret_cast<sockaddr*>(&clientAddr),
					   &clientLen);

	if (cfd < 0)
	{
		perror("accept");
		return;
	}

	char ip[INET_ADDRSTRLEN];
	std::memset(ip, 0, sizeof(ip));

	if (!inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)))
		std::strcpy(ip, "");

	pollfd p;
	p.fd = cfd;
	p.events = POLLIN;
	p.revents = 0;

	fds.push_back(p);
	clients.push_back(Client(cfd, ip, ""));

	std::cout << "Accepted client fd=" << cfd
			  << " ip=" << ip << std::endl;
}


static const char* method_to_string(RequestMethod method)
{
	if (method == METHOD_GET)
		return "GET";
	if (method == METHOD_POST)
		return "POST";
	if (method == METHOD_DELETE)
		return "DELETE";
	return "UNKNOWN";
}

static const char* request_method_to_string(requestMethod method)
{
	if (method == GET)
		return "GET";
	if (method == POST)
		return "POST";
	if (method == DELETE)
		return "DELETE";
	return "UNKNOWN";
}

static void print_http_request_debug(const HttpRequest& req)
{
	std::cout << "---- Parsed HttpRequest ----" << std::endl;
	std::cout << "method enum:   [" << method_to_string(req.method) << "]" << std::endl;
	std::cout << "method text:   [" << req.methodString << "]" << std::endl;
	std::cout << "target:        [" << req.target << "]" << std::endl;
	std::cout << "path:          [" << req.path << "]" << std::endl;
	std::cout << "query string:  [" << req.queryString << "]" << std::endl;
	std::cout << "version:       [" << req.version << "]" << std::endl;
	std::cout << "contentLength: [" << req.contentLength << "]" << std::endl;

	for (size_t i = 0; i < req.headers.size(); ++i)
	{
		std::cout << "header:        ["
				  << req.headers[i].name
				  << "] = ["
				  << req.headers[i].value
				  << "]"
				  << std::endl;
	}

	if (!req.body.empty())
		std::cout << "body:          [" << req.body << "]" << std::endl;
}

/* static void print_request_debug()
{
	std::cout << "---- Mapped Request ----" << std::endl;

} */

static void print_request_debug(const Request& req)
{
	const reqVariables& vars = req.getVariables();

	std::cout << "method:        [" << request_method_to_string(vars.method) << "]" << std::endl;
	std::cout << "contentLength: [" << vars.contentLength << "]" << std::endl;
	std::cout << "requestPath:   [" << vars.requestPath << "]" << std::endl;
	std::cout << "CONTENT_TYPE:  [" << vars.CONTENT_TYPE << "]" << std::endl;
	std::cout << "QUERY_STRING:  [" << vars.QUERY_STRING << "]" << std::endl;
	std::cout << "REMOTE_ADDR:   [" << vars.REMOTE_ADDR << "]" << std::endl;
	std::cout << "REMOTE_HOST:   [" << vars.REMOTE_HOST << "]" << std::endl;
	std::cout << "body:          [" << req.getBody() << "]" << std::endl;
	std::cout << "clientFD:      [" << req.getClientFD() << "]" << std::endl;
}

static void process_request(std::vector<pollfd>& fds,
							std::vector<Client>& clients,
							size_t idx)
{
	Client& c = clients[idx - 1];

	if (c.in.size() > 1024 * 1024 + 8192)
	{
		queue_response(fds, clients, idx,
			"HTTP/1.1 413 Payload Too Large",
			"<h1>413 Payload Too Large</h1>",
			false);
		return;
	}

	HttpRequestParser::Result status = c.parser.feed(c.in);

	if (status == HttpRequestParser::NEED_MORE)
		return;

	if (status == HttpRequestParser::PARSE_ERROR)
	{
		int err = c.parser.getErrorCode();

		if (err == 405)
		{
			queue_response(fds, clients, idx,
				"HTTP/1.1 405 Method Not Allowed",
				"<h1>405 Method Not Allowed</h1>",
				false);
		}
		else if (err == 505)
		{
			queue_response(fds, clients, idx,
				"HTTP/1.1 505 HTTP Version Not Supported",
				"<h1>505 HTTP Version Not Supported</h1>",
				false);
		}
		else if (err == 413)
		{
			queue_response(fds, clients, idx,
				"HTTP/1.1 413 Payload Too Large",
				"<h1>413 Payload Too Large</h1>",
				false);
		}
		else
		{
			queue_response(fds, clients, idx,
				"HTTP/1.1 400 Bad Request",
				"<h1>400 Bad Request</h1>",
				false);
		}
		return;
	}

	const HttpRequest& parsed = c.parser.getRequest();
	print_http_request_debug(parsed);

	Location* loc = NULL;
	Request req(loc, parsed, c.fd, c.remoteAddr, c.remoteHost);
	print_request_debug(req);

	bool keepAlive = request_wants_keep_alive(parsed);

	const char* body = "<h1>Hello World!</h1>";

	if (parsed.path == "/bye")
		body = "<h1>Bye!</h1>";
	else if (parsed.path == "/hello")
		body = "<h1>Hello!</h1>";
	else if (parsed.method == METHOD_POST)
		body = "<h1>POST received!</h1>";
	else if (!parsed.queryString.empty())
		body = "<h1>Query string received!</h1>";

	queue_response(fds, clients, idx,
		"HTTP/1.1 200 OK",
		body,
		keepAlive);
}

static void handle_client_read(std::vector<pollfd>& fds,
							   std::vector<Client>& clients,
							   size_t idx)
{
	char buf[4096];
	ssize_t n = ::read(fds[idx].fd, buf, sizeof(buf));

	if (n < 0)
	{
		if (errno == EINTR)
			return;
		perror("read");
		close_client(fds, clients, idx);
		return;
	}

	if (n == 0)
	{
		close_client(fds, clients, idx);
		return;
	}

	clients[idx - 1].in.append(buf, (size_t)n);
	process_request(fds, clients, idx);
}

static void handle_client_write(std::vector<pollfd>& fds,
								std::vector<Client>& clients,
								size_t idx)
{
	Client& c = clients[idx - 1];

	if (c.outSent >= c.out.size())
	{
		if (c.shouldClose)
		{
			close_client(fds, clients, idx);
			return;
		}

		c.out.clear();
		c.outSent = 0;
		c.shouldClose = false;
		c.keepAlive = false;
		c.parser.reset();

		set_client_events(fds, idx, POLLIN);

		if (!c.in.empty())
			process_request(fds, clients, idx);

		return;
	}

	ssize_t n = ::write(fds[idx].fd,
						c.out.data() + c.outSent,
						c.out.size() - c.outSent);

	if (n < 0)
	{
		if (errno == EINTR)
			return;
		perror("write");
		close_client(fds, clients, idx);
		return;
	}

	c.outSent += (size_t)n;

	if (c.outSent >= c.out.size())
	{
		if (c.shouldClose)
		{
			close_client(fds, clients, idx);
			return;
		}

		c.out.clear();
		c.outSent = 0;
		c.shouldClose = false;
		c.keepAlive = false;
		c.parser.reset();

		set_client_events(fds, idx, POLLIN);

		if (!c.in.empty())
			process_request(fds, clients, idx);
	}
}

static void handle_client_event(std::vector<pollfd>& fds,
								std::vector<Client>& clients,
								size_t idx)
{
	if (fds[idx].revents & POLLNVAL)
	{
		close_client(fds, clients, idx);
		return;
	}

	if (fds[idx].revents & POLLERR)
	{
		close_client(fds, clients, idx);
		return;
	}

	if (fds[idx].revents & POLLIN)
	{
		handle_client_read(fds, clients, idx);
		return;
	}

	if (fds[idx].revents & POLLOUT)
	{
		handle_client_write(fds, clients, idx);
		return;
	}

	if (fds[idx].revents & POLLHUP)
	{
		close_client(fds, clients, idx);
		return;
	}
}

static void handle_events(std::vector<pollfd>& fds,
						  std::vector<Client>& clients)
{
	if (fds[0].revents & POLLIN)
		accept_client(fds, clients);

	for (size_t i = 1; i < fds.size(); ++i)
	{
		if (fds[i].revents == 0)
			continue;

		size_t before = fds.size();
		handle_client_event(fds, clients, i);

		if (fds.size() < before)
			--i;
	}
}

static void event_loop(int listen_fd)
{
	std::vector<pollfd> fds;
	std::vector<Client> clients;

	pollfd server_pfd;
	server_pfd.fd = listen_fd;
	server_pfd.events = POLLIN;
	server_pfd.revents = 0;

	fds.push_back(server_pfd);

	while (true)
	{
		int ready = ::poll(&fds[0], (nfds_t)fds.size(), -1);

		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			perror("poll");
			return;
		}

		handle_events(fds, clients);
	}
}

int main()
{
	const int port = 6969;

	int listen_fd = make_listen_socket(port);
	if (listen_fd < 0)
	{
		perror("listen socket");
		return 1;
	}

	std::cout << "Listening on http://127.0.0.1:" << port << std::endl;

	event_loop(listen_fd);

	::close(listen_fd);
	return 0;
}
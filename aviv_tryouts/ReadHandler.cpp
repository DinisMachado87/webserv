#include "ReadHandler.hpp"
#include "Client.hpp"
#include "Server.hpp"

#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <iostream>

const char* method_to_string(RequestMethod method)
{
	if (method == METHOD_GET)
		return "GET";
	if (method == METHOD_POST)
		return "POST";
	if (method == METHOD_DELETE)
		return "DELETE";
	return "UNKNOWN";
}

const char* request_method_to_string(requestMethod method)
{
	if (method == GET)
		return "GET";
	if (method == POST)
		return "POST";
	if (method == DELETE)
		return "DELETE";
	return "UNKNOWN";
}

void print_http_request_debug(const HttpRequest& req)
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

void print_request_debug(const Request& req)
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

void process_request(std::vector<pollfd>& fds,
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

void handle_client_read(std::vector<pollfd>& fds,
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
#include "Server.hpp"
#include "Client.hpp"
#include "ReadHandler.hpp"
#include "WriteHandler.hpp"

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

int set_reuseaddr(int fd)
{
	int yes = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
					 reinterpret_cast<const void*>(&yes), sizeof(yes)) < 0)
		return -1;
	return 0;
}

int make_listen_socket(int port)
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

void close_client(std::vector<pollfd>& fds,
                  std::vector<Client>& clients,
                  size_t idx)
{
	int fd = fds[idx].fd;
	::close(fd);

	fds.erase(fds.begin() + idx);
	clients.erase(clients.begin() + (idx - 1));
}

void set_client_events(std::vector<pollfd>& fds, size_t idx, short events)
{
	fds[idx].events = events;
}

void accept_client(std::vector<pollfd>& fds,
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

void handle_client_event(std::vector<pollfd>& fds,
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

void handle_events(std::vector<pollfd>& fds,
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

void event_loop(int listen_fd)
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
/*
c++ -Wall -Wextra -Werror -std=c++98 \                
        server_main.cpp Client.cpp ReadHandler.cpp WriteHandler.cpp \
        HttpRequestParser.cpp Request.cpp \
        -o server */
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
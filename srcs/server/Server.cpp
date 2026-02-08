#include "Server.hpp"
#include "Listening.hpp"
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>

// Public constructors and destructors
Server::Server() {
	struct sockaddr_in config;
	config.sin_family = AF_INET;
	config.sin_addr.s_addr = htonl(INADDR_ANY);
	config.sin_port = htonl(8080);

	Listening::create(config);
	epoll_init();
}

Server::Server(const Server& other) {}

Server::~Server() {}

// Public Methods
void Server::epoll_init() {
	_fd_epool = epoll_create(1);
	if (_fd_epool < 0)
		throw std::runtime_error(std::string("Error Epoll_create: ") + strerror(errno));
}

#include "Listening.hpp"
#include "ASocket.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>

std::runtime_error	Listening::handleError() {
	return std::runtime_error(
		std::string("Error creating Listening Socket: ")
		+ strerror(errno));
}

// Public constructors and destructors
Listening::Listening(int fd, const Server& server):
	ASocket(fd, server) { }

Listening::~Listening() {}

// Public Methods
Listening* Listening::create(const Server& server, const Listen& listenSock) {
	struct sockaddr_in	addr = {
		AF_INET,
		htons(listenSock.getPort()),
		{listenSock.getHost()},
		{0}
	};

	int	fdSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

	int enable = 1;

	if (0 <= fdSock
		&& OK == setsockopt(fdSock, SOL_SOCKET, SO_REUSEADDR,
					  &enable, sizeof(enable))
		&& OK == bind(fdSock, (struct sockaddr*)&addr, sizeof(addr))
		&& OK == listen(fdSock, SOMAXCONN))
		return new Listening(fdSock, server);

	if (0 < fdSock)
		close(fdSock);
	throw handleError();
}

void Listening::handle(int fd) {
	(void)fd;
}



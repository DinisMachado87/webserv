#include "Connection.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>

std::runtime_error	handleError() {
	return std::runtime_error(std::string("Error creating Socket: ") + strerror(errno));
}

// Public constructors and destructors
Connection::Connection(int fd, Server& server): ASocket(fd, server) {}

Connection::~Connection() {}

// Public Methods
Connection*	Connection::create(Server& server, Listen& listenSock) {
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	if (ERR == fd) throw handleError();

	int	enable = 1;
	if (ERR == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)))
		throw handleError();

	struct sockaddr_in	addr = {
		AF_INET,
		htons(listenSock.getPort()),
		{listenSock.getHost()},
		{0}
	};
	if (ERR == bind(fd, (const struct sockaddr*)&addr, sizeof(sockaddr_in))
		|| ERR == listen(fd, SOMAXCONN))
		throw handleError();
	
	return new Connection(fd, server);
};

void Connection::handle(int events) {
	(void)events;
};

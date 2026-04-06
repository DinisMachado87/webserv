#include "Listening.hpp"
#include "ASocket.hpp"
#include "Connection.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

using std::cout;
using std::endl;
using std::runtime_error;
using std::string;
using std::stringstream;

runtime_error Listening::handleFdError(const char *errMsg, const int fdSock) {
	if (0 < fdSock)
		close(fdSock);
	return runtime_error(string(errMsg) + strerror(errno));
}

// Public constructors and destructors
Listening::Listening(const int fd, const Server &server,
					 struct sockaddr_in serverAddr) :
	ASocket(fd, server, serverAddr) {}

Listening::~Listening() {}

// Public Methods
Listening *Listening::create(const Server &server, const Listen &listenSock) {
	LOG_TITLE("CREATING SOCKET");
	LOG_SERVER("Starting Server:\n", server);

	struct sockaddr_in addr = {
		AF_INET, htons(listenSock.getPort()), {listenSock.getHost()}, {0}};

	int fdSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	if (OK <= fdSock)
		LOGSOCK(Logger::LOG, "Created listening socket with fd ", fdSock);
	else
		throw handleFdError("Error creating listening socket: ", fdSock);

	int enable = 1;
	if (OK == setNonBlocking(fdSock) &&
		OK == setsockopt(fdSock, SOL_SOCKET, SO_REUSEADDR, &enable,
						 sizeof(enable)))
		LOG(Logger::LOG, "Set Listening Socket options");
	else
		throw handleFdError(
			"Error setting Listening socket non blocking/reuseaddr: ", fdSock);

	if (OK == bind(fdSock, (struct sockaddr *)&addr, sizeof(addr)))
		LOG(Logger::LOG, "Bind socket");
	else
		throw handleFdError("Error binding Listening Socket: ", fdSock);

	if (OK == listen(fdSock, SOMAXCONN)) {
		LOGSOCKHOST(Logger::LOG, "Started listening on ", listenSock.getPort(),
					listenSock.getHost());
		return new Listening(fdSock, server, addr);
	} else
		throw handleFdError("Error starting to listen with socket ", fdSock);
}

Connection *Listening::handleIn() {
	LOGSOCK(Logger::LOG, "Handel in ", _fd);
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(_fd, (sockaddr *)&clientAddr, &clientAddrLen);
	if (ERR >= clientFd) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return NULL;
		throw handleError("Error accepting client: ");
	}

	setNonBlocking(clientFd);
	LOGSOCK(Logger::LOG, "Accepted connection on Listening socket", _fd);
	LOGSOCK(Logger::LOG, "New connection socket ", clientFd);

	return new Connection(clientFd, _server, clientAddr);
}

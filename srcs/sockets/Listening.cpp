#include "Listening.hpp"
#include "ASocket.hpp"
#include "Connection.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

using std::string;
using std::runtime_error;
using std::cout;
using std::endl;

runtime_error	Listening::handleFdError(const char *errMsg, int fdSock)
{
	if (0 < fdSock)
		close(fdSock);
	return runtime_error(string(errMsg)
		+ strerror(errno));
}

// Public constructors and destructors
Listening::Listening(int fd, const Server& server, struct sockaddr_in serverAddr):
	ASocket(fd, server, serverAddr) {}

Listening::~Listening() {}

// Public Methods
Listening* Listening::create(const Server& server, const Listen& listenSock)
{
	struct sockaddr_in	addr = {
		AF_INET,
		htons(listenSock.getPort()),
		{listenSock.getHost()},
		{0}
	};

	int	fdSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	if (OK <= fdSock)
		cout << "Created listening socket with fd " << fdSock << "\n";
	else throw handleFdError("Error creating listening socket: ", fdSock);

	int enable = 1;
	if (OK == setNonBlocking(fdSock)
		&& OK == setsockopt(fdSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)))
		cout << "Set Listening Socket options" << "\n";
	else throw handleFdError("Error setting Listening socket non blocking/reuseaddr: ", fdSock);

	if (OK == bind(fdSock, (struct sockaddr*)&addr, sizeof(addr)))
		cout << "Bind socket" << "\n";
	else throw handleFdError("Error binding Listening Socket: ", fdSock);

	if (OK == listen(fdSock, SOMAXCONN)) {
		cout << "Socket " << fdSock
			<< " started listening on Port " << listenSock.getPort()
			<< " host " << listenSock.getHost() << "\n";
		return new Listening(fdSock, server, addr);
	} else throw handleFdError("Error starting to listen with socket ", fdSock);
}

Connection* Listening::handleIn()
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(_fd, (sockaddr*)&clientAddr, &clientAddrLen);
	if (OK > clientFd)
		throw handleError("Error accepting client: ");

	setNonBlocking(clientFd);
	cout << "Accepted connection on Listening socket" << _fd
		<< "New connection socket " << clientFd << endl;

	return new Connection(clientFd, _server, clientAddr);
}

void Listening::handleOut() {}


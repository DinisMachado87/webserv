#include "ASocket.hpp"
#include "webServ.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>

using std::runtime_error;
using std::string;
using std::cout;

// Public constructors and destructors
ASocket::ASocket(int fd, const Server& server, struct sockaddr_in serverAddr):
	_ptrToSelf(this),
	_fd(fd),
	_server(server),
	_serverAddr(serverAddr) {}

ASocket::~ASocket() {
	if (_fd >= 0)
		close(_fd);
}

// Error handeling
runtime_error	ASocket::handleError(const string errMsg) {
	return runtime_error( errMsg + strerror(errno));
}

// Public Methods
int		ASocket::getFd() const { return (_fd); }
void*	ASocket::getPtrToSelf() const { return _ptrToSelf; }

int	ASocket::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFD);
	if (ERR == fcntl(fd, F_SETFD, flags | O_NONBLOCK))
		throw handleError("Error setting client sock non-blocking: ");

	return OK;
}

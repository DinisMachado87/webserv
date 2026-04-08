#include "ASocket.hpp"
#include "webServ.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>

using std::cout;
using std::runtime_error;
using std::string;

// Public constructors and destructors
ASocket::ASocket(const int fd, const Server &server,
				 struct sockaddr_in serverAddr) :
	_fd(fd),
	_server(server),
	_serverAddr(serverAddr),
	_events(0) {}

ASocket::~ASocket() {
	if (_fd >= 0)
		close(_fd);
}

// Error handeling
runtime_error ASocket::handleError(const string errMsg) {
	return runtime_error(errMsg + strerror(errno));
}

// Public Methods
int ASocket::getFd() const { return (_fd); }

int ASocket::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFD);
	if (ERR == fcntl(fd, F_SETFD, flags | O_NONBLOCK))
		throw handleError("Error setting client sock non-blocking: ");
	return OK;
}

uint32_t ASocket::trackCurEvents(uint32_t events) {
	_events = events;
	return _events;
}

// defaults for virtual methods
bool ASocket::isFull() { return false; }
void ASocket::handleOut() {};
uint32_t ASocket::getEventsNextLoop() { return EPOLLIN; }
uint32_t ASocket::getCurEvents() const { return _events; }

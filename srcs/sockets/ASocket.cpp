#include "ASocket.hpp"

// Public constructors and destructors
ASocket::ASocket(int fd, const Server& server):
	_fd(fd),
	_server(server) {}

ASocket::~ASocket() {
	if (_fd >= 0)
		close(_fd);
}

// Public Methods
int ASocket::getFd() const { return (_fd); }



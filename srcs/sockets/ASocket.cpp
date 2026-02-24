#include "ASocket.hpp"
#include <stdexcept>

// Public constructors and destructors
ASocket::ASocket(int fd): _fd(fd) {
	if (_fd < 0)
		throw std::invalid_argument("Invalid FD");
	try {
		setNonBlocking();
	} catch (...) {
		close(_fd);
		throw;
	}
}

ASocket::~ASocket() {
	if (_fd >= 0)
		close(_fd);
}

// private Methods
void ASocket::setNonBlocking() {
	int flags = fcntl(_fd, F_GETFL, 0);
	if (0 > flags
		|| 0 > fcntl(_fd, F_SETFL, flags | O_NONBLOCK))
		throw(std::runtime_error("Error fcntl"));
	
}

// Public Methods
int ASocket::getFd() const { return (_fd); }



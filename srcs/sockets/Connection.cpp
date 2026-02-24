#include "Connection.hpp"
#include "ASocket.hpp"

// Public constructors and destructors
Connection::Connection(int fd): ASocket(fd) {}

Connection::~Connection() {}

// Public Methods
void Connection::handle(int events) {
	(void)events;
};

#include "Connection.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>

using std::runtime_error;
using std::string;
using std::ifstream;

// Public constructors and destructors
Connection::Connection(int fd, const Server& server, struct sockaddr_in serverAddr):
	ASocket(fd, server, serverAddr) {}

Connection::~Connection() {}

// Public Methods
Connection*	Connection::handleIn() {
	char	buffer[CHUNK_SIZE + 1];
	size_t	bitesRead = recv(_fd, buffer, CHUNK_SIZE, 0);
	write(1, buffer, bitesRead);
	
	return NULL;
};

void		Connection::handleOut() { };

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ASocket.hpp"
#include "Server.hpp"
#include <stdint.h>
#include <sys/epoll.h>

class Connection: public ASocket {
private:
	// Explicit disables
	Connection(int fd, Server& server);
	Connection(const Connection& other);
	Connection& operator=(const Connection& other);

public:
	// Constructors and destructors
	~Connection();

	// Operators overload

	// Getters and setters

	// Methods
	static Connection*	create(Server& server, Listen& listenSock);
	void				handle(int events);
};

#endif


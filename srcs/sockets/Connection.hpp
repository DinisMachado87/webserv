#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ASocket.hpp"
#include "Server.hpp"
#include <cstddef>
#include <stdint.h>
#include <string>
#include <sys/epoll.h>

class Connection: public ASocket {
private:
	std::string	_inBuff;
	std::string	_outBuff;
	
	// Explicit disables
	Connection(int fd, const Server& server, struct sockaddr_in serverAddr);
	Connection(const Connection& other);
	Connection& operator=(const Connection& other);
	// Friends
	friend class Listening;

public:
	// Constructors and destructors
	~Connection();
	// Methods
	Connection*			handleIn();
	void				handleOut();
};

#endif


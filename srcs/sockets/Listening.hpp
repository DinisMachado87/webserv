#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

// Base class
#include "ASocket.hpp"
// Other classes within the project
#include "Connection.hpp"
// Libs
#include <stdint.h>
#include <string>
#include <cstring>
// Error handling
#include <stdexcept>
#include <cerrno>
// Networking
#include <sys/socket.h>
#include <netinet/in.h>

#define OK 0
#define BACKLOG_SIZE 128
#define DEFAULT_PROTOCOL 0

class Listening: public ASocket {
private:
	int	_port;
	std::string	_host;
	// Constructor
	Listening(int fd, int port);
	// Explicit Disables
	Listening();
	Listening(const Listening& other);
	Listening& operator=(const Listening& other);

public:
	// Constructors and destructors
	~Listening();
	// Methods
	static Listening*	create(struct sockaddr_in& config);
	void				handle(int fd);
};

#endif


#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

// Base class
#include "ASocket.hpp"
// Other classes within the project
#include "Server.hpp"
// Libs
#include <stdint.h>
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
	// Constructor
	Listening(int fd, const Server& server);
	// Explicit Disables
	Listening();
	Listening(const Listening& other);
	Listening& operator=(const Listening& other);
	// Error handeling
	static std::runtime_error	handleError();

public:
	// Constructors and destructors
	~Listening();
	// Methods
	static Listening*	create(const Server& server, const Listen& listenSock);
	void				handle(int fd);
};

#endif


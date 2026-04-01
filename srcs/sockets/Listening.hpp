#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP
// Base class
#include "ASocket.hpp"
// Other classes within the project
#include "Connection.hpp"
#include "Server.hpp"
// Libs
#include <cstring>
#include <stdint.h>
// Error handling
#include <cerrno>
#include <stdexcept>
// Networking
#include <netinet/in.h>
#include <sys/socket.h>

#define OK 0
#define BACKLOG_SIZE 128
#define DEFAULT_PROTOCOL 0

class Listening : public ASocket {
private:
	// Constructor
	Listening(const int fd, const Server &server,
			  struct sockaddr_in serverAddr);
	// Explicit Disables
	Listening();
	Listening(const Listening &other);
	Listening &operator=(const Listening &other);
	// Error Handeling
	static std::runtime_error handleFdError(const char *errMsg,
											const int fdSock);

public:
	// Constructors and destructors
	~Listening();
	// Methods
	Connection *handleIn();
	static Listening *create(const Server &server, const Listen &listenSock);
};

#endif

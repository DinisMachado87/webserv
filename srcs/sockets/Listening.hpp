#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include "ASocket.hpp"
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

#define OK 0
#define BACKLOG_SIZE 128
#define DEFAULT_PROTOCOL 0

class Listening: public ASocket {
private:
	uint32_t	_port;
	std::string	_host;
	// Constructor
	Listening(uint32_t fd, uint32_t port);
	// Explicit Disables
	Listening();
	Listening(const Listening& other);
	Listening& operator=(const Listening& other);

public:
	// Constructors and destructors
	~Listening();
	// Methods
	static Listening*	create(struct sockaddr_in& config);
	void				handle(uint32_t fd);
};

#endif


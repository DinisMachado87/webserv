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

class ListeningSocket: public ASocket {
private:
	uint32_t	_port;
	std::string	_host;
	// Constructor
	ListeningSocket(uint32_t fd, uint32_t port);
	// Explicit Disables
	ListeningSocket();
	ListeningSocket(const ListeningSocket& other);
	ListeningSocket& operator=(const ListeningSocket& other);

public:
	// Constructors and destructors
	~ListeningSocket();
	// Methods
	static ListeningSocket*	create(struct sockaddr_in& config);
	void					handle(uint32_t fd);

};

#endif


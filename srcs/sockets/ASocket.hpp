#ifndef ASOCKET_HPP
#define ASOCKET_HPP

#include <netinet/in.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <stdint.h>
#include "../parser/HttpParser.hpp"

class Server;
class Connection;

class ASocket {
private:
	// Explicit disables
	ASocket();
	ASocket(const ASocket &other);
	ASocket &operator=(const ASocket &other);

protected:
	void*				_ptrToSelf;
	int					_fd;
	const Server&		_server;
	struct sockaddr_in	_serverAddr;
	HttpParser 			_parser;
	Request*			_request;
	uint32_t			_events;
	// Constructors and destructors
	ASocket(const int fd, const Server &server, struct sockaddr_in serverAddr);
	// Error Handeling
	static std::runtime_error handleError(const std::string errMsg);

public:
	// Constructors and destructors
	virtual ~ASocket();
	// Methods
	bool isFull();
	virtual Connection *handleIn() = 0;
	virtual void handleOut();
	// Events
	virtual uint32_t getEventsNextLoop();
	uint32_t getCurEvents() const;
	uint32_t trackCurEvents(uint32_t events);
	// Getters and setters
	int getFd() const;
	// Static class methods
	static int setNonBlocking(int fd);
};

#endif

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ASocket.hpp"
#include "HttpParser.hpp"
#include "Server.hpp"
#include "Validator.hpp"
#include "webServ.hpp"
#include <cstddef>
#include <sys/epoll.h>

class Connection : public ASocket {
private:
	enum _handleInState { REQUEST, RESPONSE, INITBODY, LOOPBODY };
	HttpParser _http;
	Validator _validator;
	Response *_responses[RESPONSES_CUE_SIZE];
	size_t _cur;
	size_t _back;
	uchar _handleInState;

	// Explicit disables
	Connection(const int fd, const Server &server,
			   struct sockaddr_in serverAddr);
	Connection(const Connection &other);
	Connection &operator=(const Connection &other);
	// Friends
	friend class Listening;

public:
	// Constructors and destructors
	~Connection();
	// I/O
	Connection *handleIn();
	void handleOut();
	// Event tracking
	uint32_t getEventsNextLoop();
	bool isFull() const;
	ssize_t recvToBuffer(char *buffer);
};

#endif

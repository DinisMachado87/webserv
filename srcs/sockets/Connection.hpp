#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ASocket.hpp"
#include "HttpParser.hpp"
#include "Server.hpp"
#include "Validator.hpp"
#include <sys/epoll.h>
#include <vector>

class Connection : public ASocket {
private:
	HttpParser _http;
	Validator _validator;
	std::vector<Response *> _responses;
	Response *_curResponse;

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
	int setEpollOut() const;
};

#endif

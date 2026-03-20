#ifndef ASOCKET_HPP
#define ASOCKET_HPP

#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <stdint.h>
#include "../parser/HttpParser.hpp"
#include "../responses/Response.hpp"

class Server;
class Connection;
class Respone;
class GetRespone;

class ASocket {
private:
	// Explicit disables
	ASocket();
	ASocket(const ASocket& other);
	ASocket& operator=(const ASocket& other);

protected:
	void*				_ptrToSelf;
	int					_fd;
	const Server&		_server;
	struct sockaddr_in	_serverAddr;
	HttpParser 			_parser;
	Request*			_request;
	// Constructors and destructors
	ASocket(int fd, const Server& server, struct sockaddr_in serverAddr);
	// Error Handeling
	static std::runtime_error	handleError(const std::string errMsg);

public:
	// Constructors and destructors
	virtual	~ASocket();
	// Methods
	virtual Connection*	handleIn() = 0;
	virtual void		handleOut() = 0;
	// Getters and setters
	static int	setNonBlocking(int fd);
	int			getFd() const;
	void*		getPtrToSelf() const;
};

#endif


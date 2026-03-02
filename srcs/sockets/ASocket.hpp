#ifndef ASOCKET_HPP
#define ASOCKET_HPP

#include <unistd.h>
#include <stdint.h>

class Server;

class ASocket {
private:
	// Explicit disables
	ASocket();
	ASocket(const ASocket& other);
	ASocket& operator=(const ASocket& other);

protected:
	int		_fd;
	const Server&	_server;

	// Constructors and destructors
	ASocket(int fd, const Server& server);

public:
	// Constructors and destructors
	virtual ~ASocket();

	// Methods
	void setNonBlocking();
	int getFd() const;

	virtual void handle(int events) = 0;
};

#endif


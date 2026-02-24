#ifndef ASOCKET_HPP
#define ASOCKET_HPP

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

class ASocket {
private:
	// Explicit disables
	ASocket();
	ASocket(const ASocket& other);
	ASocket& operator=(const ASocket& other);

protected:
	int	_fd;

	// Constructors and destructors
	ASocket(int fd);

public:
	// Constructors and destructors
	virtual ~ASocket();

	// Methods
	void setNonBlocking();
	int getFd() const;

	virtual void handle(int events) = 0;
};

#endif


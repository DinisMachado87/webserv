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
	~ASocket();

	// Methods
	void setNonBlocking();

	virtual void handle(uint32_t events) = 0;
};

#endif


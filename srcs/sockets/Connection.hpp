#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ASocket.hpp"
#include <stdint.h>

class Connection: public ASocket {
private:
	// Explicit disables
	Connection();
	Connection(const Connection& other);
	Connection& operator=(const Connection& other);

public:
	// Constructors and destructors
	Connection(int fd);
	~Connection();

	// Operators overload

	// Getters and setters

	// Methods
	void handle(int events);
};

#endif


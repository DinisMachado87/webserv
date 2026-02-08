#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ASocket.hpp"
#include <cstdint>

class Connection: public ASocket {
private:
	// Explicit disables
	Connection();
	Connection(const Connection& other);
	Connection& operator=(const Connection& other);

public:
	// Constructors and destructors
	Connection(uint32_t fd);
	~Connection();

	// Operators overload

	// Getters and setters

	// Methods
	void Handle();

};

#endif


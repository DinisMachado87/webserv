#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/epoll.h>
#include <netinet/in.h>

class Server {
private:
	uint32_t _fd_epool;
	// Explicit Disables
	Server(const Server& other);
	Server& operator=(const Server& other);
	// Private Methods
	void epoll_init();
public:
	// Constructors and destructors
	Server();
	~Server();
};

#endif


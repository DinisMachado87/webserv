#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "ASocket.hpp"
#include <map>
#include <sys/epoll.h>
#include <netinet/in.h>

class Engine {
protected:
	int						_fdEpoll;
	std::map<int, ASocket*>	_sockets;

	// Explicit Disables
	Engine();
	Engine(const Engine& other);
	Engine& operator=(const Engine& other);
	// Methods
	ASocket* getSocket(int fd);
	void addSocket(ASocket* socket);
	void deleteSocket(int fd);
	void epoll_init();
public:
	// Constructors and destructors
	Engine(struct sockaddr_in& config);
	~Engine();
};

#endif


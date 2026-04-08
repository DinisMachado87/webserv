#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "ASocket.hpp"
#include "webServ.hpp"
#include <map>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <vector>

class Engine {
private:
	// Explicit Disables
	Engine(const Engine &other);
	Engine &operator=(const Engine &other);

protected:
	int _fdEpoll;
	std::vector<Server *> _servers;
	std::map<int, ASocket *> _sockets;
	// Methods
	void logFlagUpdates(ASocket *socket, uint32_t events, uint32_t newEvents);
	std::runtime_error handleError(const std::string errMsg, const int err);
	void deleteSocket(ASocket *socket);
	void updateFlags(ASocket *socket);
	void pollLoop();
	void createSockets();
	void buildServers(std::string &config);
	ASocket *getSocket(int fd);
	void setEventTo(int epollFd, uint operation, uint eventType, int socketFd,
					ASocket *ptrToSock);
	void addSocket(ASocket *socket);
	void epoll_init();

public:
	// Constructors and destructors
	Engine();
	~Engine();
	// Methods
	void run(std::string &config);
};

#ifdef LOGGING
#define LOGEVENTS(socket, events, newEvents)                                   \
	Engine::logFlagUpdates(socket, events, newEvents)
#else
#define LOGEVENTS(socket, events, newEvents) (void)0
#endif

#endif

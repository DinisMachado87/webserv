#include "Engine.hpp"
#include "ASocket.hpp"
#include "Listening.hpp"
#include <cerrno>
#include <stdint.h>
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <iostream>
#include <stdexcept>
#include <string>

// Public constructors and destructors
Engine::Engine(struct sockaddr_in& config): _fdEpoll(-1) {
	epoll_init();

	ASocket* newSocket = Listening::create(config);
	addSocket(newSocket);
}

Engine::~Engine() {
	for (std::map<int, ASocket*>::iterator socket = _sockets.begin();
		socket != _sockets.end();
		++socket) {
		delete socket->second;
	}

	if (_fdEpoll > 0)
		close(_fdEpoll);
}

// Public Methods
void Engine::epoll_init() {
	_fdEpoll = epoll_create(1);
	if (_fdEpoll < 0)
		throw std::runtime_error(
			std::string("Error Epoll_create: ")
			+ strerror(errno));
}

ASocket* Engine::getSocket(int fd) {
	std::map<int, ASocket*>::iterator socket = _sockets.find(fd);
	if (socket != _sockets.end())
		return socket->second;
	return NULL;
}

void Engine::addSocket(ASocket* socket) {
	if (!socket)
		throw std::runtime_error(
			std::string("Error null socket")
			+ strerror(errno));
	int fd = socket->getFd();
	if (socket && fd > 0)
		_sockets[fd] = socket;
	else
		throw std::runtime_error(
			std::string("Error adding socket")
			+ strerror(errno));
}

void Engine::deleteSocket(int fd) {
	std::map<int, ASocket*>::iterator socket = _sockets.find(fd);
	if (socket != _sockets.end()) {
		delete socket->second;
		_sockets.erase(socket);
	}
}

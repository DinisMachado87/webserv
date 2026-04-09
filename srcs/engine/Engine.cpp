#include "Engine.hpp"
#include "ASocket.hpp"
#include "ConfParser.hpp"
#include "Connection.hpp"
#include "Listening.hpp"
#include "Logger.hpp"
#include "webServ.hpp"
#include <bits/types/error_t.h>
#include <cerrno>
#include <cstring>
#include <exception>
#include <map>
#include <netinet/in.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <sys/epoll.h>
#include <utility>
#include <vector>

using std::exception;
using std::map;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::vector;

// Public constructors and destructors
Engine::Engine() :
	_fdEpoll(-1) {
	epoll_init();
}

Engine::~Engine() {
	map<int, ASocket *>::iterator socket = _sockets.begin();
	while (socket != _sockets.end())
		delete (socket++)->second;

	vector<Server *>::iterator server = _servers.begin();
	while (server != _servers.end())
		delete *server++;

	if (_fdEpoll > 0)
		close(_fdEpoll);
}

// Error handeling
runtime_error Engine::handleError(const string errMsg, const int err) {
	return runtime_error(errMsg + strerror(err));
}

// Public Methods
void Engine::epoll_init() {
	_fdEpoll = epoll_create(1);
	if (_fdEpoll < 0)
		throw handleError("Error Epoll_create: ", errno);
}

ASocket *Engine::getSocket(int fd) {
	map<int, ASocket *>::iterator socket = _sockets.find(fd);
	if (socket != _sockets.end())
		return socket->second;
	return NULL;
}

void Engine::setEventTo(int epollFd, uint operation, uint eventType,
						int socketFd, ASocket *socket) {
	struct epoll_event event;
	event.events = eventType;
	event.data.ptr = socket;
	if (OK == epoll_ctl(epollFd, operation, socketFd, &event))
		return;
	throw handleError("Error setting epoll socket event type: ", errno);
}

void Engine::addSocket(ASocket *socket) {
	if (!socket)
		throw handleError("Error null socket", errno);

	int fd = socket->getFd();
	if (!socket || fd < 0)
		throw handleError("Error adding socket", errno);

	_sockets[fd] = socket;
	setEventTo(_fdEpoll, EPOLL_CTL_ADD,
			   socket->trackCurEvents(EPOLLIN | EPOLLET), fd, socket);
}

void Engine::deleteSocket(ASocket *socket) {
	int fd = socket->getFd();
	setEventTo(_fdEpoll, EPOLL_CTL_DEL, 0, fd, socket);
	delete socket;
	_sockets.erase(fd);
}

void Engine::buildServers(string &config) {
	ConfParser parser(config, _servers);
	parser.createServers();
}

void Engine::createSockets() {
	vector<Server *>::iterator server = _servers.begin();
	vector<Server *>::iterator end = _servers.end();

	while (server != end) {
		vector<Listen>::iterator port = (*server)->_listen.begin();
		while (port != (*server)->_listen.end()) {
			Listening *socket = Listening::create(**server, *port);
			addSocket(socket);
			port++;
		}
		server++;
	}
}

void Engine::logFlagUpdates(ASocket *socket, uint32_t events,
							uint32_t newEvents) {
	stringstream stream;
	bool newEventsIn = newEvents & EPOLLIN;
	bool newEventsOut = newEvents & EPOLLOUT;
	bool EventsIn = events & EPOLLIN;
	bool EventsOut = events & EPOLLOUT;
	if (!newEventsIn && EventsIn)
		LOGSOCK(Logger::LOG, "Removing EPOLLIN", socket->getFd());
	else if (newEventsIn && !EventsIn)
		LOGSOCK(Logger::LOG, "Adding EPOLLIN", socket->getFd());
	if (!newEventsOut && EventsOut)
		LOGSOCK(Logger::LOG, "Removing EPOLLOUT", socket->getFd());
	else if (newEventsOut && !EventsOut)
		LOGSOCK(Logger::LOG, "Adding EPOLLOUT", socket->getFd());
}

void Engine::updateFlags(ASocket *socket) {
	uint32_t events = socket->getCurEvents();
	uint32_t newEvents = socket->getEventsNextLoop();

	LOGEVENTS(socket, events, newEvents);
	if (newEvents != events) {
		socket->trackCurEvents(newEvents);
		setEventTo(_fdEpoll, EPOLL_CTL_MOD, newEvents, socket->getFd(), socket);
	}
}

void Engine::pollLoop() {
	struct epoll_event events[MAX_EVENTS];
	int nFds = -1;

	while (!g_shutdown) {
		nFds = -1;
		nFds = epoll_wait(_fdEpoll, events, MAX_EVENTS, TIMEOUT);
		if (ERR == nFds) {
			if (errno == EINTR)
				continue;
			throw handleError("Epoll_wait error: ", errno);
		}

		for (int i = 0; i < nFds && !g_shutdown; i++) {
			ASocket *socket = static_cast<ASocket *>(events[i].data.ptr);
			uint32_t ev = events[i].events;
			try {
				if (ev & (EPOLLERR | EPOLLHUP)) {
					deleteSocket(socket);
					continue;
				}
				if (ev & EPOLLIN) {
					while (Connection *connection = socket->handleIn())
						if (connection)
							addSocket(connection);
				}
				if (ev & EPOLLOUT)
					socket->handleOut();

				updateFlags(socket);

			} catch (const exception err) {
				LOG_ERROR(runtime_error(string("Error handeling socket event: ")
										+ err.what()));
				deleteSocket(socket);
			}
		}
	}
}

void Engine::run(string &config) {
	try {
		buildServers(config);
		createSockets();
		pollLoop();
	} catch (runtime_error err) {
		LOG_ERROR(err);
	}
}

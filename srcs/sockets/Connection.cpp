#include "Connection.hpp"
#include "ASocket.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>

using std::cout;
using std::endl;
using std::fill;
using std::ifstream;
using std::runtime_error;
using std::string;

// Public constructors and destructors
Connection::Connection(const int fd, const Server &server,
					   struct sockaddr_in serverAddr) :
	ASocket(fd, server, serverAddr),
	_validator(server),
	_cur(0),
	_back(0) {
	for (size_t i = 0; i < RESPONSES_CUE_SIZE; i++)
		_responses[i] = NULL;
}

Connection::~Connection() {}

// Public Methods
Connection *Connection::handleIn() {
	char buffer[RECV_SIZE + 1];
	ssize_t bytesRead = recv(_fd, buffer, RECV_SIZE, 0);
	if (bytesRead > 0) {
		buffer[bytesRead] = '\0';
		cout << "RECV buffer: " << buffer << endl;
	} else if (bytesRead == 0) {
		return NULL;
	} else if (bytesRead >= ERR) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return NULL;
		throw runtime_error("Error: recv() failure reading from client");
	}

	Request *request = _http.parse(buffer, bytesRead);
	if (request)
		_responses[_back] = _validator.handleRequest(request);
	if (_responses[_back])
		_back = (_back + 1 % RESPONSES_CUE_SIZE);
	return NULL;
};

void Connection::handleOut() {
	if (!_responses[_cur]) {
		LOG(Logger::WARNING, "Socket handleOut without a response");
		return;
	}

	if (DONE == _responses[_cur]->sendResponse(_fd)) {
		delete _responses[_cur];
		_responses[_cur] = NULL;
		_cur = (_cur + 1) % RESPONSES_CUE_SIZE;
	}
}

uint32_t Connection::getEventsNextLoop() {
	uint32_t events = 0;
	if (_responses[_cur])
		events |= EPOLLOUT;
	if (!isFull())
		events |= EPOLLIN | EPOLLET;
	return events;
}

bool Connection::isFull() const { return (_responses[_back]); }

#include "Connection.hpp"
#include "ASocket.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

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
	_back(0),
	_handleInState(REQUEST) {
	for (size_t i = 0; i < RESPONSES_CUE_SIZE; i++)
		_responses[i] = NULL;
}

Connection::~Connection() {
	LOGSOCK(Logger::LOG, "Destroying ", _fd);
	while (_responses[_cur]) {
		_responses[_cur] = NULL;
		_cur = (_cur + 1) % RESPONSES_CUE_SIZE;
	}
}

ssize_t Connection::recvToBuffer(char *buffer) {
	ssize_t bytesRead = recv(_fd, buffer, RECV_SIZE, 0);

	if (bytesRead <= ERR && !(errno == EAGAIN || errno == EWOULDBLOCK))
		LOG_ERROR(runtime_error("recv() failure reading from client"));
	if (bytesRead <= 0)
		return 0;

	buffer[bytesRead] = '\0';
	LOG(Logger::CONTENT, "RECV buffer: ");
	LOG(Logger::CONTENT, buffer);
	return bytesRead;
}

// Public Methods
Connection *Connection::handleIn() {
	LOGSOCK(Logger::LOG, "Connection Handel in ", _fd);

	Request *request = NULL;
	char buffer[RECV_SIZE + 1];
	ssize_t bytesRead = 0;

	if (!(bytesRead = recvToBuffer(buffer)))
		return NULL;

	switch (_handleInState) {

	case (REQUEST):
		request = _http.parse(buffer, bytesRead);
		if (!request)
			return NULL;

		_responses[_back] = _validator.handleRequest(request);
		if (!_responses[_back]) {
			LOG(Logger::WARNING, "Validator did not return response");
			delete request;
			return NULL;
		}

		LOGSOCKNUM(Logger::LOG, "Storing _response on slot ", _back, _fd);
		_back = ((_back + 1) % RESPONSES_CUE_SIZE);
	// 	_handleInState = INITBODY;
	//
	// case (INITBODY): // fallthrough
	// 	if (DONE == _responses[_back]->readBodyFirst(buffer, bytesRead))
	// 		_handleInState = LOOPBODY;
	// 	return NULL;
	//
	// case (LOOPBODY):
	// 	if (DONE == _responses[_back]->readBodyLoop(buffer, bytesRead))
	// 		_handleInState = REQUEST;
	//
	default: // fallthrough
		return NULL;
	}
}

void Connection::handleOut() {
	LOGSOCK(Logger::LOG, "Connection Handel out ", _fd);
	if (!_responses[_cur]) {
		LOG(Logger::WARNING, "Socket handleOut without a response");
		return;
	}

	LOGSOCKNUM(Logger::LOG, "Sending response on slot: ", _cur, _fd);
	if (DONE == _responses[_cur]->sendResponse(_fd)) {
		LOGSOCKNUM(Logger::LOG, "DONE: Deleting response on slot: ", _cur, _fd);

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

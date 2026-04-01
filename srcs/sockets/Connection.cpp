#include "Connection.hpp"
#include "ASocket.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "webServ.hpp"
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
using std::ifstream;
using std::runtime_error;
using std::string;

// Public constructors and destructors
Connection::Connection(const int fd, const Server &server,
					   struct sockaddr_in serverAddr) :
	ASocket(fd, server, serverAddr),
	_validator(server) {}

Connection::~Connection() {}

// Public Methods
Connection *Connection::handleIn() {
	char buffer[RECV_SIZE + 1];
	size_t bytesRead = recv(_fd, buffer, RECV_SIZE, 0);

	if (bytesRead == ERR) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			throw runtime_error("Error: recv() failure reading from client");
		return NULL;
	}

	if (bytesRead <= 0)
		return NULL;
	cout << "RECV buffer: " << buffer << endl;

	buffer[bytesRead] = '\0';

	Request *request = _http.parse(buffer, bytesRead);
	if (!request)
		return NULL;

	Response *response = _validator.handleRequest(request);
	if (!response)
		return NULL;

	if (!_curResponse)
		_curResponse = response;
	else
		_responses.push_back(response);

	return NULL;
};

void Connection::handleOut() {
	if (!_curResponse) {
		if (_responses.empty())
			return; // somehow signal taking out epollout
		_curResponse = _responses.front();
	}

	if (!_curResponse->sendResponse(_fd))
		delete _curResponse; // delete is response is over
}

int Connection::setEpollOut() const {
	if ((_curResponse || !_responses.empty()) && !(_events & EPOLLOUT))
		return ADD_EPOLLOUT;
	if (!_curResponse && _responses.empty() && (_events & EPOLLOUT))
		return REMOVE_EPOLLOUT;
	return NONE;
}

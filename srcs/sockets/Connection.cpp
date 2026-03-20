#include "Connection.hpp"
#include "Server.hpp"
#include "webServ.hpp"
#include "../requests/Request.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>

using std::runtime_error;
using std::string;
using std::ifstream;

// Public constructors and destructors
Connection::Connection(int fd, const Server& server, struct sockaddr_in serverAddr):
	ASocket(fd, server, serverAddr) {}

Connection::~Connection() {}

// Public Methods
Connection*	Connection::handleIn() {
	char	buffer[CHUNK_SIZE + 1];
	ssize_t	bitesRead = recv(_fd, buffer, CHUNK_SIZE, 0);
	if (bitesRead > 0)
	{
		Request *CurrentRequest = _parser.parse(buffer, bitesRead, _fd, _server);
		while (CurrentRequest)
		{
			Response* CurrentResponse = CurrentRequest->validateAndCreateResponse();
			if (CurrentResponse)
			{
				CurrentResponse->sendResponse(_fd);
				delete CurrentResponse;
			}
			delete CurrentRequest;

			CurrentRequest = _parser.parse(NULL, 0, _fd, _server);
		}
	}
	return NULL;
};

void		Connection::handleOut() { };

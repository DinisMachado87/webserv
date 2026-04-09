#include "Server.hpp"
#include "Location.hpp"
#include "Overrides.hpp"
#include "webServ.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <vector>

using std::string;
using std::stringstream;

// Public constructors and destructors
Server::Server() :
	_defaults(_strBuf, _strvVecBuf) {};

Server::~Server() {}

// Listen Struct private methods
in_addr_t Listen::getHost() const { return _host; }
uint16_t Listen::getPort() const { return _port; }

// Private Methods
size_t Server::getLoncationsLen() { return _locations.size(); }
size_t Server::getListenLen() { return _listen.size(); }

void Server::reserve(uint sizeStrBuf, uint sizeStrvVecBuf, uint sizeintVecBuf) {
	_strBuf.reserve(sizeStrBuf);
	_strvVecBuf.reserve(sizeStrvVecBuf);
	_intVecBuf.reserve(sizeintVecBuf);
}

const char *Server::safeStr(const char *str) const {
	return str ? str : "NULL";
}

string Server::formatIP(in_addr_t addr) const {
	struct in_addr in;
	in.s_addr = addr;
	return std::string(inet_ntoa(in));
}

void Server::getServerStr(stringstream &stream) const {
	if (!LOGGING)
		return;

	stream << "----- SERVER -----" << '\n';
	stream << "\nListen addresses (" << _listen.size() << "):" << '\n';
	for (size_t i = 0; i < _listen.size(); i++) {
		stream << "  [" << i << "] Host: " << formatIP(_listen[i].getHost())
			   << ", Port: " << _listen[i].getPort() << '\n';
	}

	_defaults.printOverrides("Defaults", stream);

	stream << "\nLocations: " << '\n';
	for (size_t i = 0; i < _locations.size(); i++)
		_locations[i].printLocation(i, stream);

	printBufferSizes(stream);

	stream << "-----" << '\n';
}

void Server::printBufferSizes(stringstream &stream) const {
	stream << "\nBuffer Sizes:" << '\n';
	stream << "  String buffer: " << _strBuf.size() << "/" << _strBuf.capacity()
		   << '\n';
	stream << "  StrView buffer: " << _strvVecBuf.size() << "/"
		   << _strvVecBuf.capacity() << '\n';
	stream << "  Int buffer: " << _intVecBuf.size() << "/"
		   << _intVecBuf.capacity() << '\n';
}

#include "StrView.hpp"
#include "Token.hpp"
#include "webServ.hpp"
#include "Server.hpp"
#include <cerrno>
#include <stdexcept>
#include <stdint.h>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>


Server::Server() {};
Server::~Server() { }

// Private Methods
void Server::reserve(
	unsigned int sizeStrBuf,
	unsigned int sizeStrvVecBuf,
	unsigned int sizeintVecBuf)
{
	_strBuf.reserve(sizeStrBuf);
	_strvVecBuf.reserve(sizeStrvVecBuf);
	_intVecBuf.reserve(sizeintVecBuf);
}


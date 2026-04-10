#ifndef SERVER_HPP
#define SERVER_HPP

#include "ASocket.hpp"
#include "Location.hpp"
#include "Overrides.hpp"
#include "StrView.hpp"
#include "webServ.hpp"
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <vector>

struct Listen {
	in_addr_t _host;
	uint16_t _port;
	// Methods
	uint16_t getPort() const;
	in_addr_t getHost() const;
};

class Server {
private:
	// Explicit Disables
	Server &operator=(const Server &other);

	// Implemented for friend classes
	Server(const Server &other);

	friend class ConfParser;
	friend class ConfParserTest;

protected:
	// Contiguous Buffers
	std::string _strBuf;
	std::vector<StrView> _strvVecBuf;
	std::vector<uint> _intVecBuf;
	// Private methods
	std::string formatIP(in_addr_t addr) const;
	size_t getListenLen();
	size_t getLoncationsLen();
	void printBufferSizes(std::stringstream &stream) const;
	void printLocation(const Location &loc, size_t index,
					   std::stringstream &stream) const;
	void printOverrides(const Overrides &over, const char *label,
						std::stringstream &stream) const;
	const char *safeStr(const char *str) const;

public:
	// Constructors and destructors
	Server();
	~Server();

	std::vector<Listen> _listen;
	std::vector<Location> _locations;
	Overrides _defaults;
	// Methods
	void reserve(uint sizeStrBuf, uint sizeStrvVecBuf, uint sizeintVecBuf);
	// Getters Server Vars
	void getServerStr(std::stringstream &stream) const;
};

#endif

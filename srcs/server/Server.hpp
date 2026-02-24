#ifndef SERVER_HPP
#define SERVER_HPP

#include "ASocket.hpp"
#include "Listening.hpp"
#include "Span.hpp"
#include "StrView.hpp"
#include "Token.hpp"
#include <map>
#include <string>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <vector>

struct Overrides {
	StrView							_root;
	std::map<unsigned int, StrView>	_error;
	bool							_autoindex;
	Span<StrView>					_index;
	size_t							_clientMaxBody;
};

struct Location {
	enum	_e_allowed_methods {
		DEFAULT,
		GET,
		POST,
		PUT,
		DELETE
	};
	Overrides		_overrides;
	Span<StrView>	_cgiExtensions;
	StrView			_cgiPath;
	StrView			_path;
	StrView			_returnPath;
	StrView			_rewrite[2];
	StrView			_uploadPath;
	int	   			_returnCode;
	bool   			_uploadEnable;
	char   			_allowedMethods;
};

struct Listen {
	in_addr_t	host;
	uint16_t	port;
};

class Server {
private:
	// Explicit Disables
	Server(const Server& other);
	Server& operator=(const Server& other);

protected:
	// Contiguous Buffers
	std::string					_strBuf;
	std::vector<StrView>		_strvVecBuf;
	std::vector<unsigned int>	_intVecBuf;
	// Private vars
	std::vector<Listen>			_listen;
	std::vector<Location>		_locations;
	Overrides					_defaults;
	// Explicit disables
	friend class ConfParser;

public:
	// Methods
	void reserve(
		unsigned int sizeStrBuf,
		unsigned int sizeStrvVecBuf,
		unsigned int sizeintVecBuf);

	// Constructors and destructors
	Server();
	~Server();
};

#endif


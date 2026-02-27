#ifndef SERVER_HPP
#define SERVER_HPP

#include "webServ.hpp"
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
	// Constructor
	Overrides(std::string& buffer, std::vector<StrView>& vecBuf);
	// Vars
	std::map<unsigned int, StrView>	_error;
	StrView							_root;
	bool							_autoindex;
	Span<StrView>					_index;
	size_t							_clientMaxBody;
	// Consolidate buffer method
	void consolidateStrBuffer(std::string& newBuffer);
	// Getters
	size_t			getClientMaxBody() const;
	const char*		findErrorFile(uint errorCode) const;
	bool			isAutoindexed() const;
	const char*		getRoot() const;
	const Span<StrView>&	getIndex() const;
};

struct Location {
	// Construnctor
	Location(std::string& strBuf, std::vector<StrView>& vecBuf, Overrides* serverDefaults);

	enum	_e_allowed_methods {
		DEFAULT,
		GET,
		POST,
		PUT,
		DELETE
	};
	// Substructs
	Overrides		_overrides;
	// Pointer to server defaults for comparison getters
	Overrides*		_serverDefaults;
	// Member vars
	Span<StrView>	_cgiExtensions;
	Span<StrView>	_cgiPath;
	StrView			_path;
	StrView			_returnPath;
	StrView			_rewrite_old;
	StrView			_rewrite_new;
	StrView			_uploadPath;
	uint			_returnCode;
	bool			_uploadEnable;
	uchar   		_allowedMethods;
	//Consolidate buffer method
	void	consolidateStrBuffer(std::string& newBuffer);
	// Getters Location Vars
	uchar		isAllowedMethod(unsigned char methodToCheck) const;
	const char*	findCgiPath(StrView& extention) const;
	const char*	findCgiPath(const char* extention) const;
	const char*	getPath() const;
	const char*	getUploadPath() const;
	const char*	getRewriteNewPath() const;
	const char*	getRewriteOldPath() const;
	const char*	getReturnPath() const;
	uint		getReturncode() const;
	bool		getUploadEnabled() const;
};

struct Listen {
	in_addr_t	_host;
	uint16_t	_port;
	// Methods
	uint16_t	getPort() const;
	in_addr_t	getHost() const;
};

class Server {
private:
	// Explicit Disables
	Server& operator=(const Server& other);

	// Implemented for friend classes
	Server(const Server& other);

	friend class ConfParser;
	friend class ConfParserTest;

protected:
	// Contiguous Buffers
	std::string					_strBuf;
	std::vector<StrView>		_strvVecBuf;
	std::vector<unsigned int>	_intVecBuf;
	// Private vars

public:
	// Constructors and destructors
	Server();
	~Server();

	std::vector<Listen>			_listen;
	std::vector<Location>		_locations;
	Overrides					_defaults;
	// Methods
	void reserve(
		unsigned int sizeStrBuf,
		unsigned int sizeStrvVecBuf,
		unsigned int sizeintVecBuf);

	void	consolidateStrBuffer(std::string& newBuffer);
	// Getters Server Vars
	size_t	getListenLen();
	size_t	getLoncationsLen();
};

#endif


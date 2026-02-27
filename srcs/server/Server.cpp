#include "StrView.hpp"
#include "Token.hpp"
#include "webServ.hpp"
#include "Server.hpp"
#include <cerrno>
#include <cstddef>
#include <stdint.h>
#include <map>
#include <stdexcept>
#include <stdint.h>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <vector>

// Public constructors and destructors
Overrides::Overrides(std::string& strBuf, std::vector<StrView>& vecBuf) :
	_root(strBuf),
	_autoindex(false),
	_index(vecBuf),
	_clientMaxBody(0) {}

Location::Location(std::string& strBuf, std::vector<StrView>& vecBuf, Overrides* serverDefaults) :
	_overrides(strBuf, vecBuf),
	_serverDefaults(serverDefaults),
	_cgiExtensions(vecBuf),
	_cgiPath(vecBuf),
	_path(strBuf),
	_returnPath(strBuf),
	_rewrite_old(StrView(strBuf)),
	_rewrite_new(StrView(strBuf)),
	_uploadPath(strBuf),
	_returnCode(0),
	_uploadEnable(false),
	_allowedMethods(DEFAULT) {}

Server::Server(): _defaults(_strBuf, _strvVecBuf) {};

Server::~Server() { }

// Private Methods
void Server::reserve(
	uint sizeStrBuf,
	uint sizeStrvVecBuf,
	uint sizeintVecBuf)
{
	_strBuf.reserve(sizeStrBuf);
	_strvVecBuf.reserve(sizeStrvVecBuf);
	_intVecBuf.reserve(sizeintVecBuf);
}

size_t	Server::getLoncationsLen() { return _locations.size(); }
size_t	Server::getListenLen() { return _listen.size(); }

const Span<StrView>&	Overrides::getIndex() const	{ return _index; };
const char*		Overrides::getRoot() const			{ return _root.getStart(); };
bool			Overrides::isAutoindexed() const	{ return _autoindex; };
size_t			Overrides::getClientMaxBody() const	{ return _clientMaxBody; };

const char*		Overrides::findErrorFile(uint errorCode) const {
	std::map<unsigned int, StrView>::const_iterator it = _error.find(errorCode);
	return (it != _error.end()) ? it->second.getStart() : NULL;
};

const char*	Location::getPath() const			{ return _path.getStart(); }
const char*	Location::getReturnPath() const		{ return _returnPath.getStart(); }
const char*	Location::getRewriteOldPath() const	{ return _rewrite_old.getStart(); }
const char*	Location::getRewriteNewPath() const	{ return _rewrite_new.getStart(); }
const char*	Location::getUploadPath() const		{ return _uploadPath.getStart(); }
uint		Location::getReturncode() const		{ return _returnCode; }
bool		Location::getUploadEnabled() const	{ return _uploadEnable; }

const char*	Location::findCgiPath(StrView& extention) const{
	return findCgiPath(extention.getStart());
}

const char*	Location::findCgiPath(const char* extention) const {
	for(uint i = 0; i < _cgiExtensions.len() ; i++)
		if (_cgiExtensions[i].compare(extention))
			return _cgiPath[i].getStart();
	return NULL;
}

uchar	Location::isAllowedMethod(unsigned char methodToCheck) const {
	return _allowedMethods & (1 << methodToCheck);
};

in_addr_t Listen::getHost() const		{ return _host; }
uint16_t	Listen::getPort() const	{ return _port; }



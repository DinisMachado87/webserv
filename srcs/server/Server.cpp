#include "Server.hpp"
#include "StrView.hpp"
#include "Token.hpp"
#include "webServ.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <ostream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <vector>

using std::cout;
using std::endl;
using std::string;

// Public constructors and destructors
Overrides::Overrides(std::string &strBuf, std::vector<StrView> &vecBuf) :
	_root(strBuf),
	_autoindex(false),
	_index(vecBuf),
	_clientMaxBody(0) {}

Location::Location(std::string &strBuf, std::vector<StrView> &vecBuf,
				   Overrides *serverDefaults) :
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

Location &Location::operator=(const Location &other) {
	if (this == &other) {
		return *this;
	}
	_overrides = other._overrides;
	_serverDefaults = other._serverDefaults;
	_cgiExtensions = other._cgiExtensions;
	_cgiPath = other._cgiPath;
	_path = other._path;
	_returnPath = other._returnPath;
	_rewrite_old = other._rewrite_old;
	_rewrite_new = other._rewrite_new;
	_uploadPath = other._uploadPath;
	_returnCode = other._returnCode;
	_uploadEnable = other._uploadEnable;
	_allowedMethods = other._allowedMethods;
	return *this;
}

Server::Server() :
	_defaults(_strBuf, _strvVecBuf) {};

Server::~Server() {}

// Private Methods
void Server::reserve(uint sizeStrBuf, uint sizeStrvVecBuf, uint sizeintVecBuf) {
	_strBuf.reserve(sizeStrBuf);
	_strvVecBuf.reserve(sizeStrvVecBuf);
	_intVecBuf.reserve(sizeintVecBuf);
}

size_t Server::getLoncationsLen() { return _locations.size(); }
size_t Server::getListenLen() { return _listen.size(); }

const Span<StrView> &Overrides::getIndex() const { return _index; };
const char *Overrides::getRoot() const { return _root.getStart(); };
bool Overrides::isAutoindexed() const { return _autoindex; };
size_t Overrides::getClientMaxBody() const { return _clientMaxBody; };
size_t Overrides::getErrorMapSize() const { return _error.size(); };

const char *Overrides::findErrorFile(uint errorCode) const {
	std::map<unsigned int, StrView>::const_iterator it = _error.find(errorCode);
	return ((it != _error.end()) ? it->second.getStart() : NULL);
};

const char *Location::getPath() const { return _path.getStart(); }
const char *Location::getReturnPath() const { return _returnPath.getStart(); }
const char *Location::getRewriteOldPath() const {
	return _rewrite_old.getStart();
}
const char *Location::getRewriteNewPath() const {
	return _rewrite_new.getStart();
}
const char *Location::getUploadPath() const { return _uploadPath.getStart(); }
uint Location::getReturncode() const { return _returnCode; }
bool Location::getUploadEnabled() const { return _uploadEnable; }
const Span<StrView> &Location::getCgiExtensions() const {
	return _cgiExtensions;
}
const Span<StrView> &Location::getCgiPath() const { return _cgiPath; }
const Overrides &Location::getOverrides() const { return _overrides; }

const char *Location::findCgiPath(StrView &extention) const {
	return findCgiPath(extention.getStart());
}

const char *Location::findCgiPath(const char *extention) const {
	for (uint i = 0; i < _cgiExtensions.len(); i++)
		if (_cgiExtensions[i].compare(extention))
			return _cgiPath[i].getStart();
	return NULL;
}

uchar Location::isAllowedMethod(uchar methodToCheck) const {
	return _allowedMethods & (1 << methodToCheck);
};

in_addr_t Listen::getHost() const { return _host; }

uint16_t Listen::getPort() const { return _port; }
void Server::print() const {
	cout << "----- SERVER -----" << endl;

	cout << "\nListen addresses (" << _listen.size() << "):" << endl;
	for (size_t i = 0; i < _listen.size(); i++) {
		cout << "  [" << i << "] Host: " << formatIP(_listen[i].getHost())
			 << ", Port: " << _listen[i].getPort() << endl;
	}

	printOverrides(_defaults, "Defaults");

	cout << "\nLocations (" << _locations.size() << "):" << endl;
	for (size_t i = 0; i < _locations.size(); i++) {
		printLocation(_locations[i], i);
	}

	printBufferSizes();

	cout << "-----" << endl;
}

void Server::printOverrides(const Overrides &over, const char *label) const {
	cout << "\n" << label << ":" << endl;
	cout << "  Root: " << safeStr(over.getRoot()) << endl;
	cout << "  Autoindex: " << (over.isAutoindexed() ? "true" : "false")
		 << endl;
	cout << "  Client Max Body: " << over.getClientMaxBody() << endl;
	cout << "  Index files: " << over.getIndex().len() << endl;
	cout << "  Error pages: " << over.getErrorMapSize() << endl;
}

void Server::printLocation(const Location &loc, size_t index) const {
	cout << "  [" << index << "] Path: " << safeStr(loc.getPath()) << endl;
	cout << "      Return Code: " << loc.getReturncode() << endl;
	cout << "      Return Path: " << safeStr(loc.getReturnPath()) << endl;
	cout << "      Upload Enabled: "
		 << (loc.getUploadEnabled() ? "true" : "false") << endl;
	cout << "      Upload Path: " << safeStr(loc.getUploadPath()) << endl;
	cout << "      CGI Extensions: " << loc.getCgiExtensions().len() << endl;
	cout << "      Allowed Methods: " << static_cast<int>(loc._allowedMethods)
		 << endl;
}

void Server::printBufferSizes() const {
	cout << "\nBuffer Sizes:" << endl;
	cout << "  String buffer: " << _strBuf.size() << "/" << _strBuf.capacity()
		 << endl;
	cout << "  StrView buffer: " << _strvVecBuf.size() << "/"
		 << _strvVecBuf.capacity() << endl;
	cout << "  Int buffer: " << _intVecBuf.size() << "/"
		 << _intVecBuf.capacity() << endl;
}

const char *Server::safeStr(const char *str) const {
	return str ? str : "NULL";
}

string Server::formatIP(in_addr_t addr) const {
	struct in_addr in;
	in.s_addr = addr;
	return std::string(inet_ntoa(in));
}

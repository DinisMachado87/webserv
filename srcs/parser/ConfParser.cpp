#include "ConfParser.hpp"
#include "Server.hpp"
#include "StrView.hpp"
#include "Token.hpp"
#include "webServ.hpp"
#include <arpa/inet.h>
#include <cctype>
#include <climits>
#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>

using std::cerr;
using std::endl;
using std::map;
using std::pair;
using std::string;
using std::vector;
typedef pair<map<uint, StrView>::iterator, bool> errorVecPair;

// Public constructors and destructors
ConfParser::ConfParser(string &configStr, vector<Server *> &servers) :
	_servers(servers),
	_newServer(new Server()),
	_newLocation(_newServer->_strBuf, _newServer->_strvVecBuf,
				 &_newServer->_defaults),
	_curStrConfig(configStr.c_str()),
	_vecCursor(0),
	_token(Token::configDelimiters(), configStr),
	_expect(_token) {
	_newServer->reserve(configStr.length() * 0.6, 10, 10);
};

ConfParser::~ConfParser() {}

// Err Handeling
std::runtime_error ConfParser::parsingErr(const char *expected) const {
	std::ostringstream oss;
	oss << "Error Parsing config: "
		<< "Expected \"" << expected << "\" "
		<< "got \"" << _token.getString() << "\" "
		<< "in line " << _token.getLineN() << "\"";

	return std::runtime_error(oss.str());
}

// Private Methods
void ConfParser::parseMethod() {
	uchar method;
	while (1) {
		_token.loadNext();
		switch (_token.getType()) {
		case Token::SEMICOLON:
			if (_newLocation._allowedMethods == Location::DEFAULT)
				throw parsingErr("Method definition");
			return;

		case Token::WORD:
			method = _expect.method();
			if (!method)
				throw parsingErr("Unknown method");
			else
				_newLocation._allowedMethods |= (1 << method);
			break;

		default:
			throw parsingErr("Method definition");
		}
	}
}

bool ConfParser::parseOverrides(Overrides &overrides) {
	if (_token.compare("root"))
		_expect.path(&overrides._root);
	else if (_token.compare("autoindexing"))
		overrides._autoindex = _expect.onOff();
	else if (_token.compare("index")) {
		overrides._index = _expect.wordVec(_newServer->_strvVecBuf, _vecCursor);
		return true;
	} else if (_token.compare("client_max_body_size"))
		overrides._clientMaxBody = _expect.size();
	else if (_token.compare("error_page"))
		_expect.errorPage(overrides._error, _newServer->_strBuf);
	else
		return false;

	_token.loadNextOfType(Token::SEMICOLON, "';'");
	return true;
}

void ConfParser::parseLocationParam() {
	if (_token.compare("allowed_methods")) {
		parseMethod();
		return;
	} else if (_token.compare("return")) {
		_newLocation._returnCode = _expect.nextInteger();
		_expect.path(&_newLocation._returnPath);
	} else if (_token.compare("rewrite")) {
		_expect.path(&_newLocation._rewrite_old);
		_expect.path(&_newLocation._rewrite_new);
	} else if (_token.compare("upload_enable"))
		_newLocation._uploadEnable = _expect.onOff();
	else if (_token.compare("upload_path"))
		_expect.path(&_newLocation._uploadPath);
	else if (_token.compare("cgi_extension")) {
		_newLocation._cgiExtensions
			= _expect.wordVec(_newServer->_strvVecBuf, _vecCursor);
		return;
	} else if (_token.compare("cgi_path")) {
		_newLocation._cgiPath
			= _expect.wordVec(_newServer->_strvVecBuf, _vecCursor);
		return;
	} else if (parseOverrides(_newLocation._overrides))
		return;
	else
		throw parsingErr("Unknown directive");
	_token.loadNextOfType(Token::SEMICOLON, "';'");
}

void ConfParser::parseLocation() {
	_expect.path(&_newLocation._path);
	_token.loadNextOfType(Token::OPENBLOCK, "{");

	while (1) {
		_token.loadNext();
		switch (_token.getType()) {
		case Token::CLOSEBLOCK:
			_token.consolidateBuffer(_newServer->_strBuf);
			_newServer->_locations.push_back(_newLocation);
			_newLocation
				= Location(_newServer->_strBuf, _newServer->_strvVecBuf,
						   &_newServer->_defaults);
			return;
		case Token::WORD:
			parseLocationParam();
			continue;
		case Token::ENDOFILE:
			throw parsingErr("}");
		}
	}
}

void ConfParser::parseServerLine() {
	if (_token.compare("listen")) {
		_token.loadNextOfType(Token::WORD, "listen address");

		Listen listen;
		string portStr = _token.getString();
		string ipStr = "*";

		// in case ip:port extracts ip
		size_t colonPos = portStr.find(':');
		if (colonPos != string::npos) {
			ipStr = portStr.substr(0, colonPos);
			portStr = portStr.substr(colonPos + 1);
		}

		listen._host = _expect.ip(ipStr);
		listen._port = _expect.port(portStr);

		_token.loadNextOfType(Token::SEMICOLON, "';'");
		_newServer->_listen.push_back(listen);
	} else if (!parseOverrides(_newServer->_defaults))
		throw parsingErr("Unknown directive");
}

void ConfParser::nextServer() {
	while (1) {
		switch (_token.loadNext()) {
		case Token::WORD:
			if (_token.compare("location"))
				parseLocation();
			else
				parseServerLine();
			continue;
		case Token::CLOSEBLOCK:
			_token.consolidateBuffers(_newServer->_strvVecBuf,
									  _newServer->_strBuf);
			_servers.push_back(_newServer);
			_newServer = new Server();
			_vecCursor = 0;
			_token.resetSpanConsolidationIndex();
			return;
		default:
			throw parsingErr("Unexpected token");
		}
	}
}

void ConfParser::createServers() {
	while (1) {
		switch (_token.loadNext()) {
		case Token::WORD:
			if (_token.compare("server")) {
				_token.loadNextOfType(Token::OPENBLOCK, "{");
				nextServer();
			} else
				throw parsingErr("\"server\"");
			break;
		case Token::ENDOFILE:
			return;
		default:
			throw parsingErr("{");
		}
	}
}

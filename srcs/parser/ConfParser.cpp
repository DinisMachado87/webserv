#include "ConfParser.hpp"
#include "Server.hpp"
#include "StrView.hpp"
#include "Token.hpp"
#include "webServ.hpp"
#include <cctype>
#include <climits>
#include <cstddef>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <utility>
#include <vector>

// Public constructors and destructors
ConfParser::ConfParser(std::string configStr, std::vector<Server*>& servers):
	_servers(servers),
	_newServer(new Server()),
	_newLocation(_newServer->_strBuf, _newServer->_strvVecBuf,&_newServer->_defaults),
	_curStrConfig(configStr.c_str()),
	_vecCursor(0),
	_token(Token::configDelimiters(), _newServer->_strBuf),
	_curType(0),
	_expect(_token, _curType)
{
	_newServer->reserve(configStr.length() * 0.6, 10, 10);
};

ConfParser::~ConfParser() {}

// Public Methods

std::runtime_error ConfParser::parsingErr(const char* expected) const {
	std::ostringstream oss;
	oss << "Error Parsing config: "
		<< "Expected \"" << expected << "\" "
		<< "got \"" << _token.getString() << "\" "
		<< "in line " << _token.getLineN() << "\"";

	return std::runtime_error(oss.str());
}

// Public Methods
void	ConfParser::parseMethod() {
	const char *methods[] = {"DEFAULT", "GET", "POST", "PUT", "DELETE"};
	const unsigned char size = 5;

	while (1) {
		_token.next();
		switch (_token.getType()) {
			case Token::SEMICOLON:
				if (_newLocation._allowedMethods == Location::DEFAULT)
					throw parsingErr("Method definition");
				return;
			case Token::WORD: {
				for (int i = 1; i < size; i++) {
					if (OK == _token.compare(methods[i])) {
						_newLocation._allowedMethods |= (1 << i);
						goto method_found;
					}
				}
				throw parsingErr("Unknown method");
				
				method_found:
				break;
			}
			default:
				throw parsingErr("Method definition");
		}
	}
}

bool	ConfParser::parseOverrides(Overrides& overrides) {
	if (_token.compare("root"))
		overrides._root = _expect.path();
	else if (_token.compare("autoindexing"))
		overrides._autoindex = _expect.onOff();
	else if (_token.compare("index"))
		overrides._index = _expect.wordVec(_newServer->_strvVecBuf, _vecCursor);
	else if (_token.compare("client_max_body_size"))
		overrides._clientMaxBody = _expect.size();
	else if (_token.compare("error_page")) {
		unsigned int code = _expect.integer();
		StrView path = _expect.path();
		overrides._error.insert(std::make_pair(code, path));
	} else return false;
	return true;
}

void	ConfParser::parseLocationParam() {
	if (_token.compare("allowed_methods"))
		parseMethod();
	else if (_token.compare("return")) {
		_newLocation._returnCode = _expect.integer();
		_newLocation._returnPath = _expect.path();
	} else if (_token.compare("rewrite")){
		_newLocation._rewrite_old = _expect.path();
		_newLocation._rewrite_new = _expect.path();
	}
	else if (_token.compare("upload_enable"))
		_newLocation._uploadEnable = _expect.onOff();
	else if (_token.compare("upload_path"))
		_newLocation._uploadPath = _expect.path();
	else if (_token.compare("cgi_extension"))
		_newLocation._cgiExtensions = _expect.wordVec(_newServer->_strvVecBuf, _vecCursor);
	else if (_token.compare("cgi_path"))
		_newLocation._cgiPath = _expect.wordVec(_newServer->_strvVecBuf, _vecCursor);
	else if (!parseOverrides(_newLocation._overrides))
		throw parsingErr("Unknown directive");
}

void	ConfParser::parseLocation() {
	_newLocation._path = _expect.path();
	_token.getNextOfType(Token::OPENBLOCK, "{");

	while (1) {
		_token.next();
		switch (_token.getType()) {
			case Token::CLOSEBLOCK:
				_newServer->_locations.push_back(_newLocation);
				return;
			case Token::WORD:
				parseLocationParam();
				continue;
			case Token::ENDOFILE:
				throw parsingErr("}");
		}
	}
}

void	ConfParser::parseServerLine() {
	if (_token.compare("listen")) {
		int num1 = _expect.integer();
		int num2 = _expect.integer();
		Listen listen;
		listen._host = num1;
		listen._port = num2;

		_newServer->_listen.push_back(listen);
	}
	else if (!parseOverrides(_newServer->_defaults))
		throw parsingErr("Unknown directive");
}

void	ConfParser::nextServer() {
	while (1) {
		_token.next();
		_curType = _token.getType();

		if (Token::WORD == _curType) {
			if (_token.compare("location"))
				parseLocation();
			else parseServerLine();
		}
		else if (Token::CLOSEBLOCK == _curType) {
			_token.consolidateBuffer(_newServer->_strBuf);
			_servers.push_back(_newServer);
			_newServer = new Server();
			return;
		}
		else throw parsingErr("Unexpected token");
	}
}

void	ConfParser::createServers() {
	while (1) {
		switch (_token.next()) {
			case Token::WORD:
				if (_token.compare("server"))
					_token.getNextOfType(Token::OPENBLOCK, "{");
				else throw parsingErr("\"server\"");
				break;
			case Token::ENDOFILE:
				return;
			default: throw parsingErr("{");

			nextServer();
		}
	}
}

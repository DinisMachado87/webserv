#include "webServ.hpp"
#include "Expect.hpp"
#include "StrView.hpp"
#include "Token.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using std::map;
using std::string;
using std::pair;

// Public constructors and destructors
Expect::Expect(Token& token, uchar& curType):
	_token(token),
	_curType(curType) {
}

Expect::~Expect() {}

// Error Handler
std::runtime_error Expect::parsingErr(const char* expected) const {
	std::ostringstream oss;
	oss << "Error Parsing config: "
		<< "Expected \"" << expected << "\" "
		<< "got \"" << _token.getString() << "\" "
		<< "in line " << _token.getLineN() << "\"";

	return std::runtime_error(oss.str());
}

// Public Methods

bool	Expect::onOff() {
	_token.getNextOfType(Token::WORD, "\"on/off\"");
	if (_token.compare("on"))
		return (true);
	if (_token.compare("off"))
		return (false);
	throw parsingErr("\"on/off\"");
}

uchar	Expect::word(const char *str) {
	_token.getNextOfType(Token::WORD, str);
	if (!_token.compare(str))
		throw parsingErr(str);
	return Token::WORD;
}

Span<StrView>	Expect::wordVec(std::vector<StrView>& vecBuf, uint& vecCursor) {
	uint count = 0;
	while (1) {
		switch (_token.next()) {
			case Token::WORD:
				vecBuf.push_back(_token.getStrV());
				_token.trackInUseToken(&vecBuf.back());
				count++;
				break;
			case Token::SEMICOLON: {
				Span<StrView> ret(vecBuf, vecCursor, count);
				vecCursor += count;
				return ret;
			}
			default: throw parsingErr("WORD");
		}
	}
}

void	Expect::errorPage(map<uint, StrView>& errorMap, string& strBuf)
{
	uint code = nextInteger();

	// Empty StrView placeholder
	pair<uint, StrView> placeholderEntry(code, StrView(strBuf));

	// Insert empty StrView placeholder to reserve slot in map
	// and pass a stable pointer rather than a copy to _expect.path();
	// insert() returns pair<iterator, bool> where:
	//   .first  = iterator pointing to the element (inserted or existing)
	//   .second = true if inserted, false if key already existed
	pair<map<uint, StrView>::iterator, bool> insertResult = 
		errorMap.insert(placeholderEntry);

	// Get pointer to the StrView now stored in the map
	map<uint, StrView>::iterator errorIter = insertResult.first;
	StrView* pathPtr = &errorIter->second;

	// Fill the empty StrView and register it for consolidation
	path(pathPtr);
}

void	Expect::path(StrView* dest) {
	_token.getNextOfType(Token::WORD, "/<PATH>");
	if (_token.getStrV().getStart()[0] == '/') {
		*dest = _token.getStrV();
		_token.trackInUseToken(dest);
		return;
	}
	throw parsingErr("/<PATH>");
}

void	Expect::paths(StrView* paths, int n) {
	for (int i = 0; i < n; i++) {
		path(&paths[i]);
	}
}

size_t Expect::applySizeUnit(size_t value, char unit) {
	if (unit == '\0') return value;

	size_t multiplier;
	switch (tolower(unit)) {
		case 'k': multiplier = 1024; break;
		case 'm': multiplier = 1024 * 1024; break;
		case 'g': multiplier = 1024 * 1024 * 1024; break;
		default: throw parsingErr("Invalid size unit (use k, m, or g)");
	}

	if (value > std::numeric_limits<size_t>::max() / multiplier)
		throw parsingErr("Size value too large");
	return value * multiplier;
}

long Expect::number(const char** endPtr) {
	StrView token = _token.getStrV();
	const char* start = token.getStart();
	const char* tokenEnd = start + token.getLen();

	errno = 0;
	char* parseEnd;
	long result = strtol(start, &parseEnd, 10);

	if (errno == ERANGE) throw parsingErr("Number out of range");
	if (parseEnd == start) throw parsingErr("Expected number");
	if (parseEnd > tokenEnd) throw parsingErr("Invalid number format");
	if (result < 0) throw parsingErr("Negative number not allowed");

	*endPtr = parseEnd;
	return result;
}

int Expect::integer() {
	const char* end;
	long result = number(&end);

	StrView token = _token.getStrV();
	if (end != token.getStart() + token.getLen())
		throw parsingErr("Unexpected characters after number");
	if (result > INT_MAX)
		throw parsingErr("Number exceeds INT_MAX");

	return static_cast<int>(result);
}

int Expect::nextInteger() {
	_token.getNextOfType(Token::WORD, "word");
	return integer();
}

size_t Expect::size() {
	_token.getNextOfType(Token::WORD, "word");

	const char* end;
	long result = number(&end);
	size_t size = static_cast<size_t>(result);
	StrView token = _token.getStrV();
	const char* tokenEnd = token.getStart() + token.getLen();

	if (end != tokenEnd) {
		size = applySizeUnit(size, *end);
		if (++end != tokenEnd)
			throw parsingErr("Invalid characters after size unit");
	}
	return size;
}

in_addr_t Expect::ip(string& ipStr) {
	const size_t nOctets = 4;

	if (ipStr == "*" || ipStr == "0.0.0.0")
		return INADDR_ANY;

	if (ipStr == "localhost")
		ipStr = "127.0.0.1";

	uchar octets[nOctets];
	size_t start = 0;

	for (size_t i = 0; i < nOctets; i++) {
		size_t dotPos = (i < 3) ? ipStr.find('.', start) : ipStr.length();

		if (dotPos == string::npos || dotPos == start) // npos == not found/no position
			throw parsingErr("Invalid IP address");

		string octetStr = ipStr.substr(start, dotPos - start);

		char* end;
		errno = 0;
		long octet = strtol(octetStr.c_str(), &end, 10);

		if (errno == ERANGE || *end != '\0' || octet < 0 || octet > 255)
			throw parsingErr("Invalid IP address");

		octets[i] = static_cast<uchar>(octet);
		start = dotPos + 1;
	}

	uint32_t result = 0;
	result |= octets[0];
	result |= octets[1] << 8;
	result |= octets[2] << 16;
	result |= octets[3] << 24;
	return static_cast<in_addr_t>(result);
}

uint16_t Expect::port(const string& portStr) {
	if (portStr.empty())
		throw parsingErr("Invalid port number");

	char* end;
	errno = 0;
	long port = strtol(portStr.c_str(), &end, 10);

	if (errno == ERANGE || *end != '\0' || port < 1 || port > 65535)
		throw parsingErr("Port must be between 1 and 65535");

	return static_cast<uint16_t>(port);
}

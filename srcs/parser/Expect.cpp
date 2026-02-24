
#include "Expect.hpp"
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <vector>

// Public constructors and destructors

Expect::Expect(Token& token, const char*& curStr, unsigned char& curType):
	_token(token),
	_curStr(curStr),
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
	_curStr = _token.next(_curStr);

	if (Token::WORD != _token.getType()) {
		if (_token.compare("on")) return (true);
		if (_token.compare("off")) return (false);
	}
	throw parsingErr("\"on/off\"");
}

void	Expect::word(const char *str1 = NULL) {
	_curStr = _token.next(_curStr);

	if (Token::WORD != _token.getType())
		throw parsingErr(str1 ? str1 : "type WORD");

	if (str1 && !_token.compare(str1))
		throw parsingErr(str1);
}

Span<StrView>	Expect::wordVec(std::vector<StrView>& vecBuf, unsigned int& vecCursor) {
	int i = 0;

	while (1) {
		_curStr = _token.next(_curStr);

		_curType = _token.getType();
		if (Token::WORD == _curType) {
			vecBuf.push_back(_token.getStrV());
			i++;
		}
		else if (Token::SEMICOLON == _curType)
			return Span<StrView>(&vecBuf, vecCursor, i);
		else throw parsingErr("WORD");
	}
}

void	Expect::type(unsigned char type, const char *str) {
	_curStr = _token.next(_curStr);

	if (type != _token.getType())
		throw parsingErr(str);
}

size_t Expect::applySizeUnit(size_t value, char unit) {
	size_t sizeTMax = std::numeric_limits<size_t>::max(); 

	if (unit == '\0')
		return value;

	size_t multiplier;
	switch (tolower(unit)) {
		case 'k': multiplier = 1024; break;
		case 'm': multiplier = 1024 * 1024; break;
		case 'g': multiplier = 1024 * 1024 * 1024; break;
		default: throw parsingErr("Invalid size unit (use k, m, or g)");
	}

	if (value > sizeTMax / multiplier)
		throw parsingErr("Size value too large");

	return value * multiplier;
}


long Expect::number(const char** endPtr) {
	word();

	StrView token = _token.getStrV();
	const char* start = token.getStart();
	const char* tokenEnd = start + token.getLen();
	char* parseEnd;

	errno = 0;
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
	const char* tokenEnd = token.getStart() + token.getLen();

	if (end != tokenEnd)
		throw parsingErr("Unexpected characters after number");
	if (result > INT_MAX)
		throw parsingErr("Number exceeds INT_MAX");

	return static_cast<int>(result);
}

size_t Expect::size() {
	size_t sizeTMax = std::numeric_limits<size_t>::max(); 
	const char* end;
	long result = number(&end);

	if (static_cast<unsigned long>(result) > sizeTMax)
		throw parsingErr("Size exceeds SIZE_MAX");

	size_t size = static_cast<size_t>(result);

	StrView token = _token.getStrV();
	const char* tokenEnd = token.getStart() + token.getLen();

	if (end != tokenEnd) {
		size = applySizeUnit(size, *end);
		end++;
		if (end != tokenEnd)
			throw parsingErr("Invalid characters after size unit");
	}

	return size;
}

StrView	Expect::path() {
	_curStr = _token.next(_curStr);
	if (_token.getType() == Token::WORD) {
		StrView strv = _token.getStrV();
		if (strv.getStart()[0] == '/')
			return strv;
	}
	throw parsingErr("/<PATH>");
}

void	Expect::paths(StrView* paths, int n) {
	for (int i = 0; i < n; i++) {
		paths[i] = path();
	}
}



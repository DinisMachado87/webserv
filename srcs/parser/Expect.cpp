
#include "Expect.hpp"
#include "StrView.hpp"
#include "Token.hpp"
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <vector>

// Public constructors and destructors

Expect::Expect(Token& token, unsigned char& curType):
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

unsigned char	Expect::word(const char *str) {
	_token.getNextOfType(Token::WORD, str);
	if (!_token.compare(str))
		throw parsingErr(str);
	return Token::WORD;
}

Span<StrView>	Expect::wordVec(std::vector<StrView>& vecBuf, unsigned int& vecCursor) {
	unsigned int count = 0;
	while (1) {
		switch (_token.next()) {
			case Token::WORD:
				vecBuf.push_back(_token.getStrV());
				_token.trackInUseToken(_token.getStrV());
				count++;
				break;
			case Token::SEMICOLON:
				return Span<StrView>(vecBuf, vecCursor, count);
			default: throw parsingErr("WORD");
		}
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
	_token.getNextOfType(Token::WORD, "word");
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

size_t Expect::size() {
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
StrView	Expect::path() {
	word("/<PATH>");
	StrView strv = _token.getStrV();
	if (strv.getStart()[0] == '/') {
		_token.trackInUseToken(strv);
		return strv;
	}
	throw parsingErr("/<PATH>");
}

void	Expect::paths(StrView* paths, int n) {
	for (int i = 0; i < n; i++) {
		paths[i] = path();
	}
}

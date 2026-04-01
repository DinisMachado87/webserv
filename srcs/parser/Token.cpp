#include "Token.hpp"
#include "Server.hpp"
#include "StrView.hpp"
#include "webServ.hpp"
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

using std::string;
using std::strtol;
using std::vector;

// Public constructors and destructors
Token::Token(const uchar *table, std::string &parsingString) :
	_isDelimiter(table),
	_strV(StrView(parsingString)),
	_type(0),
	_lineN(0),
	_pendingQuote(false),
	_strBuffSize(0) {}

Token::~Token() {}

// Error Handling
std::runtime_error Token::parsingErr(const char *expected) const {
	std::ostringstream oss;
	oss << "Error Parsing config: "
		<< "Expected \"" << expected << "\" "
		<< "got \"" << getString() << "\" "
		<< "in line " << getLineN() << "\"";

	return std::runtime_error(oss.str());
}

// Public Methods
const uchar *Token::configDelimiters() {
	static uchar isDelimiter[256] = {0};
	isDelimiter[' '] = SPACE;
	isDelimiter['\t'] = SPACE;
	isDelimiter['\n'] = SPACE;
	isDelimiter['#'] = COMMENT;
	isDelimiter['"'] = QUOTE;
	isDelimiter['{'] = OPENBLOCK;
	isDelimiter['}'] = CLOSEBLOCK;
	isDelimiter[';'] = SEMICOLON;
	isDelimiter['\\'] = EXCAPE;
	isDelimiter['\0'] = ENDOFILE;
	return isDelimiter;
}

void Token::extractQuote(const char *str) {
	str++;

	while (1) {
		_type = _isDelimiter[(uchar)(*str)];
		switch (_type) {
		case NEWLINE:
		case ENDOFILE:
			throw std::runtime_error("Error tokenizer: unclosed quote");
		case EXCAPE:
			str++;
			if (*str)
				str++;
			continue;
		case QUOTE:
			_strV.setLen(str - _strV.getStart());
			_pendingQuote = true;
			return;
		default:
			str++;
		}
	}
}

uchar Token::loadNext() { return loadNextCore(false); }

uchar Token::loadNextStr() { return loadNextCore(true); }

uchar Token::loadNextCore(const bool keepSpaces) {
	_strV.updateOffset(_strV.getLen() + _pendingQuote);
	_pendingQuote = false;
	const char *str = _strV.getStart();
	_strV.setLen(0);

	while (1) {
		_type = _isDelimiter[(uchar)(*str)];
		switch (_type) {
		case NEWLINE: // extract new line expresion
			_strV.setStart(str);
			while (NEWLINE == _isDelimiter[(uchar)(*str)])
				str++;
			_strV.setLen(str - _strV.getStart());
			return _type;
		case SPACE: // Skip cursor
			str++;
			break;
		case COMMENT: // Skip comment
			while (*str && *str != '\n')
				str++;
			break;

		case WORD: // Extract Token
			_strV.setStart(str);
			if (keepSpaces)
				while (WORD == _isDelimiter[(uchar)(*str)] ||
					   SPACE == _isDelimiter[(uchar)(*str)])
					str++;
			else
				while (WORD == _isDelimiter[(uchar)(*str)])
					str++;
			_strV.setLen(str - _strV.getStart());
			return _type;

		case QUOTE:
			str++;
			_strV.setStart(str);
			extractQuote(str);
			return _type = WORD;

		default:
			_strV.setStart(str);
			_strV.setLen(1);
			return _type;
		}
	}
}

uchar Token::loadNextOfType(uchar type, const char *errStr) {
	loadNext();
	if (type != _type)
		throw parsingErr(errStr);
	return _type;
}

uchar Token::loadNextStr(const char *errStr) {
	loadNextStr();
	if (WORD != _type)
		throw parsingErr(errStr);
	return _type;
}

uchar Token::loadNextOfTypes(uchar *types, uint nTypes, const char *errStr) {
	loadNext();

	while (nTypes--) {
		if (*types == _type)
			return *types;
		types++;
	}
	throw parsingErr(errStr);
}

bool Token::loadNextHex(size_t *ret) {
	static const int hexLen = 16;
	static const bool needsMoreInput = true;
	_strV.updateOffset(_strV.getLen());
	const char *start = _strV.getStart();

	errno = 0;
	char *end;
	long num = strtol(start, &end, hexLen);

	if (errno == ERANGE)
		throw parsingErr("Number out of range");
	if (num < 0)
		throw parsingErr("Negative chunk len received");

	// Check if there is space to be "\r\n" or "\0\r\n" terminated
	size_t len = end - start;
	size_t sizeLeft = getSizeLeft() - len;
	if ((*end == '\0' && 3 > sizeLeft) || 2 > sizeLeft)
		return needsMoreInput;

	if (end == start)
		throw parsingErr("Expected number");

	_strV.setLen(len);

	if (!loadHttpNewLine() || NEWLINE != getType())
		throw parsingErr("Newline");

	*ret = static_cast<size_t>(num);
	return !needsMoreInput;
}

uchar Token::loadHttpNewLine() {
	_strV.updateOffset(_strV.getLen());
	_strV.setLen(0);

	size_t sizeLeft = getSizeLeft();
	if (sizeLeft < 2)
		return OTHER;

	if (_strV.compare("\r\n")) {
		_strV.setLen(2);
		return NEWLINE;
	} else if (sizeLeft < 2)
		return OTHER;
	else if (_strV.compare("\0\r\n")) {
		_strV.setLen(3);
		return ENDOFILE;
	}
	throw parsingErr("End of line or request");
}

bool Token::compare(const char *str) const {
	const uchar len = _strV.getLen();
	if (OK == strncmp(_strV.getStart(), str, len) && str[len] == '\0')
		return true;
	return false;
};

bool Token::compare(StrView &strV) const { return compare(strV.getStart()); };

char Token::compare(const char **strArr, uchar len) {
	for (uchar i = 0; i < len; i++)
		if (OK == compare(strArr[i]))
			return i;
	return -1;
}

uchar Token::getType() const { return _type; }
StrView Token::getStrV() const { return _strV; }
int Token::getLineN() const { return _lineN; }
const char *Token::getStart() const { return _strV.getStart(); }
const char *Token::getEnd() const { return _strV.getEnd(); }

size_t Token::getSizeLeft() const {
	return (_strV.getBufferSize() - _strV.getOffset() - _strV.getLen());
}

StrView Token::getRemaining() {
	_strV.updateOffset(_strV.getLen());
	_strV.setStart(_strV.getStart());
	_strV.setLen(getSizeLeft());
	return _strV;
}

void Token::loadNextChunk(const size_t size) {
	_strV.updateOffset(_strV.getLen());
	_strV.setStart(_strV.getStart());
	_strV.setLen(size);
}

string Token::getString() const {
	return (string(_strV.getStart(), _strV.getLen()));
}

void Token::trackInUseToken(StrView *strV) {
	_tokensInUse.push_back(strV);
	_strBuffSize += strV->getLen() + 1;
}

void Token::consolidateStrVSpans(vector<StrView> &vecBuf, string &newStrBuf) {
	std::cout << "Consolidating StrView Span Buffer: " << std::endl;

	size_t i = _vecBuffConsolidationIndex;
	for (; i < vecBuf.size(); i++)
		vecBuf[i].move(newStrBuf);
	_vecBuffConsolidationIndex = vecBuf.size();
}

void Token::consolidateBuffer(string &newBuf) {
	std::cout << "Consolidating StrView Buffer: " << std::endl;

	for (uint i = 0; i < _tokensInUse.size(); i++) {
		_tokensInUse[i]->printStrV();
		_tokensInUse[i]->move(newBuf);
	}
	_tokensInUse.clear();
}

void Token::consolidateBuffers(vector<StrView> &vecBuf, string &newStrBuf) {
	newStrBuf.reserve(newStrBuf.size() + _strBuffSize);
	consolidateBuffer(newStrBuf);
	consolidateStrVSpans(vecBuf, newStrBuf);
	_strBuffSize = 0;
}

void Token::resetSpanConsolidationIndex() { _vecBuffConsolidationIndex = 0; }

void Token::LoadParsingString(string &parsingString) {
	_strV.setBuffer(parsingString);
}

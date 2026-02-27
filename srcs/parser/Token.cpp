#include "Token.hpp"
#include "StrView.hpp"
#include "webServ.hpp"
#include <cctype>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

// Public constructors and destructors
Token::Token(const unsigned char* table, std::string& parsingString):
	_isDelimiter(table),
	_strV(StrView(parsingString)),
	_type(0),
	_lineN(0),
	_pendingQuote(false),
	_strBuffSize(0) {}

Token::~Token() { }

// Error Handling
std::runtime_error Token::parsingErr(const char* expected) const {
	std::ostringstream oss;
	oss << "Error Parsing config: "
		<< "Expected \"" << expected << "\" "
		<< "got \"" << getString() << "\" "
		<< "in line " << getLineN() << "\"";

	return std::runtime_error(oss.str());
}

// Public Methods
const unsigned char* Token::configDelimiters() {
	static unsigned char isDelimiter[256] = {0};
	isDelimiter[' '] = SPACE;
	isDelimiter['\t'] = SPACE;
	isDelimiter['\n'] = NEWLINE;
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
		_type = _isDelimiter[(unsigned char)(*str)];
		switch (_type) {
			case NEWLINE :
			case ENDOFILE :
				throw std::runtime_error("Error tokenizer: unclosed quote");
			case EXCAPE :
				str++;
				if (*str)
					str++;
				continue;
			case QUOTE :
				_strV.setLen(str - _strV.getStart());
				_pendingQuote = true;
				return;
			default : str++;
		}
	}
}

unsigned char Token::next() {
	_strV.updateOffset(_strV.getLen() + _pendingQuote);
	_pendingQuote = false;
	const char *str	= _strV.getStart();
	_strV.setLen(0);

	while (1) {
		_type = _isDelimiter[(unsigned char)(*str)];
		switch (_type) {
			case NEWLINE : // Skip and count '\n'
				_lineN++;
				str++;
				break;
			case SPACE : // Skip cursor
				str++;
				break;
			case COMMENT : // Skip comment
				while (*str && *str != '\n')
					str++;
				break;

			case WORD : // Extract Token
				_strV.setStart(str);
				while (WORD == _isDelimiter[(unsigned char)(*str)])
					str++;
				_strV.setLen(str - _strV.getStart());
				return _type;

			case QUOTE :
				str++;
				_strV.setStart(str);
				extractQuote(str);
				return _type = WORD;

			default : // Extract other single char delimiters
				_strV.setStart(str);
				_strV.setLen(1);
				return _type;
		}
	}
}

unsigned char	Token::getNextOfType(unsigned char type, const char *errStr) {
	next();

	if (type != _type)
		throw parsingErr(errStr);
	return _type;
}

unsigned char	Token::getNextOfTypes(unsigned char* types, unsigned int nTypes, const char *errStr) {
	next();

	while (nTypes--) {
		if (*types == _type)
			return *types;
		types++;
	}
	throw parsingErr(errStr);
}

bool Token::compare(const char *str) const {
	const unsigned char len = _strV.getLen();
	if (OK == strncmp(_strV.getStart(), str, len)
		&& str[len] == '\0')
		return true;
	return false;
};

bool Token::compare(StrView& strV) const {
	return compare(strV.getStart());
};

char Token::compare(const char** strArr, unsigned char len) {
	for (unsigned char i = 0; i < len; i++)
		if (OK == compare(strArr[i]))
			return i;
	return -1;
}

unsigned char	Token::getType() const { return (_type); }
StrView			Token::getStrV() const { return _strV; }
int				Token::getLineN() const { return _lineN; }

std::string		Token::getString() const {
	return (std::string(_strV.getStart(), _strV.getLen()));
}

void	Token::trackInUseToken(const StrView& strV) {
	_tokensInUse.push_back(&_strV);
	_strBuffSize += strV.getLen() + 1 ;
}

void	Token::consolidateBuffer(std::string& newBuffer) {
	newBuffer.reserve(_strBuffSize);
	for (unsigned int i = 0; i < _tokensInUse.size() ; i++)
		_tokensInUse[i]->move(newBuffer);
	_tokensInUse.clear();
}

void	Token::LoadParsingString(std::string& parsingString) {
	_strV.setBuffer(parsingString);
}

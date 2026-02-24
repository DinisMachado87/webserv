#include "Token.hpp"
#include "StrView.hpp"
#include "webServ.hpp"
#include <cctype>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>

// Public constructors and destructors
Token::Token(const unsigned char* table, std::string& buffer):
	_isDelimiter(table),
	_strV(StrView(&buffer)),
	_type(0),
	_lineN(0) {}

Token::~Token() { }

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

const char* Token::extractQuote(const char *str) {
	str++;
	_strV.setStart(str);
	while (1) {
		_type = _isDelimiter[(unsigned char)(*str)];
		switch (_type) {

			case NEWLINE :
			case ENDOFILE :
				throw std::runtime_error("Error tokenizer: unclosed quote");

			case QUOTE :
				_strV.setLen(str - _strV.getStart());
				str++;
				return str;

			case EXCAPE :
				str++;
				if (*str)
					str ++;
				continue;

			default :
				str++;
		}
	}
}
const char* Token::next(const char *str) {
	_strV.setLen(0);
	_strV.setStart(NULL);

	while (1) {
		_type = _isDelimiter[(unsigned char)(*str)];
		switch (_type) {
			case ENDOFILE :
				return (NULL);

			case NEWLINE : // Skip and count '\n'
				_lineN++;
				str++;
				break;

			case SPACE : // Skip spaces
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
				return str;

			case QUOTE : // Extract quoted token
				return (extractQuote(str));

			default : // Extract other single char delimiters
				_strV.setStart(str);
				_strV.setLen(1);
				return ++str;
		}
	}
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

unsigned char Token::getType() const { return (_type); }

std::string	Token::getString() const {
	return (std::string(_strV.getStart(), _strV.getLen()));
}

StrView	Token::getStrV() const { return _strV; }
int		Token::getLineN() const { return _lineN; }

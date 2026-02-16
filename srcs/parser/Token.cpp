#include "Token.hpp"
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <unistd.h>


// Public constructors and destructors
Token::Token(const unsigned char* table):
	_isDelimiter(table),
	_start(NULL),
	_len(0),
	_type(0),
	_lineN(0) {}

Token::~Token() {}

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
	_start = str;
	while (1) {
		_type = _isDelimiter[(unsigned char)(*str)];
		switch (_type) {
			case NEWLINE :
			case ENDOFILE :
				throw std::runtime_error("Error tokenizer: unclosed quote");
			case QUOTE :
				_len = str - _start;
				str++;
				return str;
			case EXCAPE :
				if (*str && *(str + 1))
					str += 2;
				continue;
			default :
				str++;
		}
	}
}
const char* Token::next(const char *str) {
	_len = 0;
	_start = NULL;

	while (1) {
		_type = _isDelimiter[(unsigned char)(*str)];
		switch (_type) {
			case ENDOFILE :
				return (NULL);
			// Skip new line;
			case NEWLINE : 
				_lineN++;
				str++;
				break;
			// Skip Spaces;
			case SPACE :  
				str++;
				break;
			// Skip comments;
			case COMMENT :
				while (*str && *str != '\n')
					str++;
				break;
			// Extract word
			case WORD :
				_start = str;
				while (*str && WORD == _isDelimiter[(unsigned char)(*str)])
					str++;
				_len = str - _start;
				return str;
			case QUOTE :
				return (extractQuote(str));
			// Extract other single char delimiters
			default :
				_start = str;
				_len = 1;
				return ++str;
		}
	}
}

void Token::printToken() {
	write(1, "\t", 1);
	for(unsigned char i = 0; i < _len; i++)
		write(1, &_start[i], 1);
	write(1, "\n", 1);
}

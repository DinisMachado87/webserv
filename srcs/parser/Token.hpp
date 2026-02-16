#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <sys/types.h>

class Token {
protected:
	const unsigned char* const	_isDelimiter;

	const char*		_start;
	unsigned char	_len;
	int				_type;
	int				_lineN;

	enum e_delimiters{
		WORD,
		SPACE,
		NEWLINE,
		SEMICOLON,
		COMMENT,
		QUOTE,
		OPENBLOCK,
		CLOSEBLOCK,
		ENDOFILE,
		EXCAPE
	};

private:
	// Explicit disables
	Token(const Token& other);
	Token& operator=(const Token& other);

public:
	// Constructors and destructors
	Token(const unsigned char* table);
	~Token();

	// Static Method for table generation
	static const unsigned char* configDelimiters();
	// Methods
	const char* extractQuote(const char *str);
	void printToken();
	const char* next(const char *str);

};

#endif


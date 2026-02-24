#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "StrView.hpp"
#include <sys/types.h>

class Token {
protected:
	const unsigned char* const	_isDelimiter;

	StrView			_strV;
	unsigned char	_type;
	int				_lineN;

private:
	// Explicit disables
	Token();
	Token(const Token& other);
	Token& operator=(const Token& other);

public:
	// Constructors and destructors
	Token(const unsigned char* table, std::string& buffer);
	~Token();

	enum e_Types {
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

	// Static Method for table generation
	static const unsigned char* configDelimiters();

	// Methods
	char			compare(const char** strArr, unsigned char len);
	bool			compare(unsigned char type, const char *str1, const char *str2) const;
	std::string		getString() const;
	bool			compare(StrView& strV) const;
	bool			compare(const char *str) const;
	const char*		extractQuote(const char *str);
	void			printToken() const;
	const char*		next(const char *str);
	// geters
	unsigned char	getType() const;
	StrView			getStrV() const;
	int				getLineN() const;
};

#endif


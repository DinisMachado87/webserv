#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "StrView.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

class Token {
protected:
	const unsigned char* const	_isDelimiter;

	StrView			_strV;
	unsigned char	_type;
	int				_lineN;
	bool			_pendingQuote;

	size_t					_strBuffSize;
	std::vector<StrView*>	_tokensInUse;

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
	unsigned char 		next();
	unsigned char		getNextOfTypes(unsigned char* types, unsigned int nTypes, const char *errStr);
	unsigned char		getNextOfType(unsigned char type, const char *errStr);
	std::runtime_error	parsingErr(const char* expected) const;
	void				LoadParsingString(std::string& parsingString);
	char				compare(const char** strArr, unsigned char len);
	std::string			getString() const;
	bool				compare(StrView& strV) const;
	bool				compare(const char *str) const;
	void				extractQuote(const char *str);
	void				printToken() const;
	void				trackInUseToken(const StrView& strV);
	void				consolidateBuffer(std::string& newBuffer);
	// geters
	unsigned char		getType() const;
	StrView				getStrV() const;
	int					getLineN() const;
};

#endif


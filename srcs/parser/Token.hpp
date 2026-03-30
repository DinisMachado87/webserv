#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "StrView.hpp"
#include "webServ.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

class Token {
protected:
	const uchar *const _isDelimiter;

	StrView _strV;
	uchar _type;
	int _lineN;
	bool _pendingQuote;

	size_t _strBuffSize;
	std::vector<StrView *> _tokensInUse;

private:
	// Explicit disables
	Token();
	Token(const Token &other);
	Token &operator=(const Token &other);

public:
	// Constructors and destructors
	Token(const uchar *table, std::string &buffer);
	~Token();

	enum e_Types {
		WORD,
		SPACE,
		NEWLINE,
		SEMICOLON,
		COMA,
		COMMENT,
		QUOTE,
		OPENBLOCK,
		CLOSEBLOCK,
		ENDOFILE,
		DIGIT,
		EXCAPE,
		OTHER
	};

	// Static Method for table generation
	static const uchar *configDelimiters();

	// Methods
	uchar loadHttpNewLine();
	void loadNextChunk(const size_t size);
	bool loadNextHex(size_t *ret);
	void loadDigitsUntil(const char c);
	StrView getRemaining();
	size_t getSizeLeft() const;
	const char *getEnd() const;
	uchar loadNextStr(const char *errStr);
	uchar loadNextStr();
	uchar loadNextCore(const bool keepSpaces);
	uchar loadNext();
	uchar loadNextOfTypes(uchar *types, uint nTypes, const char *errStr);
	uchar loadNextOfType(uchar type, const char *errStr);
	std::runtime_error parsingErr(const char *expected) const;
	void LoadParsingString(std::string &parsingString);
	char compare(const char **strArr, uchar len);
	bool compare(StrView &strV) const;
	bool compare(const char *str) const;
	void extractQuote(const char *str);
	void printToken() const;
	void trackInUseToken(StrView *strV);
	void consolidateBuffer(std::string &newBuffer);
	// geters
	const char *getStart() const;
	uchar getType() const;
	StrView getStrV() const;
	int getLineN() const;
	std::string getString() const;
};

#endif

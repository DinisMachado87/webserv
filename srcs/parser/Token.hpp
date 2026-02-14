#ifndef TOKEN_HPP
#define TOKEN_HPP

class Token {
protected:
	const unsigned char* const	_isDelimiter;

	const char*	_start;
	int			_len;
	int			_lineN;

private:
	// Explicit disables
	Token(const Token& other);
	Token& operator=(const Token& other);

public:
	enum e_delimiters{
		WORD,
		SPACE,
		ENDLINE,
		QUOTE,
		OPENBLOCK,
		CLOSEBLOCK,
		COMMENT
	};

	// Constructors and destructors
	Token(const unsigned char* table);
	~Token();

	// Static Method for table generation
	static const unsigned char* configDelimiters();
	// Methods
	void printToken();
	inline bool is(const unsigned char condition, const char c);
	const char* next(const char *str);

};

#endif


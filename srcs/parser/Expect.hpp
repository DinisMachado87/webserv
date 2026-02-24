#ifndef EXPECT_HPP
#define EXPECT_HPP

#include "Span.hpp"
#include "Token.hpp"
#include <cstddef>
#include <vector>

class Expect {
private:
	Token&				_token;
	const char*&		_curStr;
	unsigned char&		_curType;

	// Explicit disables
	Expect(const Expect& other);
	Expect& operator=(const Expect& other);

	// Helpers
	long				number(const char** endPtr);
	size_t				applySizeUnit(size_t value, char unit);
	std::runtime_error	parsingErr(const char* expected) const;

public:
	// Constructors and destructors
	Expect(Token& token, const char*& curStr, unsigned char& curType);
	~Expect();

	// Methods
	void			type(unsigned char type, const char* str);
	void			word(const char *str1);
	Span<StrView>	wordVec(std::vector<StrView>& vecBuf, unsigned int& vecCursor);
	bool			onOff();
	int				integer();
	size_t			size();
	void			paths(StrView* paths, int n);
	StrView			path();
};

#endif


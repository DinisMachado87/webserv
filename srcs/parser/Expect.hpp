#ifndef EXPECT_HPP
#define EXPECT_HPP

#include "Span.hpp"
#include "Token.hpp"
#include <cstddef>
#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

class Expect {
private:
	Token&				_token;

	// Explicit disables
	Expect(const Expect& other);
	Expect& operator=(const Expect& other);

	// Helpers
	long				number(const char** endPtr);
	size_t				applySizeUnit(size_t value, char unit);

public:
	// Constructors and destructors
	Expect(Token& token);
	~Expect();

	// Methods
	std::runtime_error	parsingErr(const char* expected) const;
	uchar				method();
	uint16_t			port(const std::string& portStr);
	in_addr_t			ip(std::string& ipStr);
	int					nextInteger();
	void				errorPage(std::map<uint, StrView>& errorMap, std::string& strBuf);
	unsigned char		word(const char *str1);
	Span<StrView>		wordVec(std::vector<StrView>& vecBuf, uint& vecCursor);
	bool				onOff();
	int					integer();
	size_t				size();
	void				paths(StrView* paths, int n);
	void				path(StrView* dest);
};

#endif


// Tester
#include <gtest/gtest.h>
// Other classes in the project
#include "Token.hpp"
#include "StrView.hpp"
#include "debug.hpp"
// Library imports
#include <cstring>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define RESET   "\033[0m"
#define PURPLE  "\033[1;35m"
#define ORANGE  "\033[1;33m"

#define NO_COLOR 0
#define COLOR 1

class TestableToken: public Token {
public:
	using		Token::_strV;
	using		Token::_lineN;
	std::string	_strBuffer;

	TestableToken(const unsigned char* config): Token(config, _strBuffer) {};
	~TestableToken() {};
};

class TokenConfigTest: public ::testing::Test {
protected:
	TestableToken* _token;

	void SetUp() { _token = new TestableToken(Token::configDelimiters()); }
	void TearDown() { delete _token; }

	void tokenInfoStream(const char* const color, std::string str, std::string token, std::ostringstream& stream) {
		stream << color
			<< "\n\t|" << str << ": \"" << token
			<< "\"\t|len: " << token.length()
			<< RESET;
	}

	std::string tokensComparison(std::string expToken, std::string curToken, bool color) {
		std::ostringstream tokensComparison;
		tokensComparison
			<< "\n\n\tLine: " << _token->_lineN;
		tokenInfoStream((color ? PURPLE : RESET), "expected", expToken, tokensComparison);
		tokenInfoStream((color ? ORANGE : RESET), "received", curToken, tokensComparison);
		return (tokensComparison.str());
	}

	void testTokenSequence(const char* inputStr, std::string* expectedTokens) {
		int i = 0;

		while(*inputStr) {
			const char* curStr = inputStr;
			inputStr = _token->next(inputStr);
			if (!inputStr)
				break;

			std::string expToken = expectedTokens[i];
			std::string curToken(_token->_strV.getStart(), _token->_strV.getLen());
			if (DEBUG_TEST)
				std::cout << tokensComparison(expToken, curToken, NO_COLOR);

			EXPECT_EQ(_token->_strV.getLen(), expToken.length());
			ASSERT_STREQ(curToken.c_str(), expToken.c_str())
				<< tokensComparison(expToken, curToken, COLOR)
				<< PURPLE << "\n\n\t|cur input str: " << curStr << RESET;
			i++;
		}
		if (DEBUG_TEST)
			std::cout << "\n\n";
	}
};

TEST_F(TokenConfigTest, consecutiveDelimiters) {
	std::string testStr("{{{");
	std::string expectedTokens[] = {"{", "{", "{"};
	testTokenSequence(testStr.c_str(), expectedTokens);
};

TEST_F(TokenConfigTest, withQuotes) {
	std::string testStr("something \"in quotes followed by \" unquoted");
	std::string expectedTokens[] = {"something", "in quotes followed by ", "unquoted"};
	testTokenSequence(testStr.c_str(), expectedTokens);
};

TEST_F(TokenConfigTest, unclosedQuotes) {
	std::string testStr("something \"  unclosed ");
	const char* inputStr = testStr.c_str();
	inputStr = _token->next(inputStr);
	ASSERT_THROW(_token->next(inputStr), std::runtime_error);
}

TEST_F(TokenConfigTest, Basic) {
	std::string testStr(" \nserver { \n\tlisten 8080; \n\tserver_name localhost; \n\troot /var/www/html; \n\tindex index.html; \n\tclient_max_body_size 1M; \n \n\terror_page 404 /404.html; \n \n\tlocation / { \n\t\tallowed_methods GET POST; \n\t\tautoindex on; \n\t} \n \n\tlocation /upload { \n\t\tallowed_methods POST DELETE; \n\t\tupload_path /var/www/uploads; \n\t} \n \n\tlocation .php { \n\t\tcgi_pass /usr/bin/php-cgi; \n\t} \n}");

	std::string expectedTokens[] = {"server", "{", "listen", "8080", ";", "server_name", "localhost", ";", "root", "/var/www/html", ";", "index", "index.html", ";", "client_max_body_size", "1M", ";", "error_page", "404", "/404.html", ";", "location", "/", "{", "allowed_methods", "GET", "POST", ";", "autoindex", "on", ";", "}", "location", "/upload", "{", "allowed_methods", "POST", "DELETE", ";", "upload_path", "/var/www/uploads", ";", "}", "location", ".php", "{", "cgi_pass", "/usr/bin/php-cgi", ";", "}", "}"};

	testTokenSequence(testStr.c_str(), expectedTokens);
}

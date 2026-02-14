#include <cstddef>
#include <cstring>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include "Token.hpp"

#define RESET   "\033[0m"
#define PURPLE  "\033[1;35m"
#define ORANGE  "\033[1;33m"

class TestableToken: public Token {
public:
	using Token::_len;
	using Token::_start;
	using Token::_lineN;

	TestableToken(const unsigned char* config): Token(config) {};
	~TestableToken() {};
};

class TokenConfigTest: public ::testing::Test {
protected:
	TestableToken* _token;

	void SetUp() { _token = new TestableToken(Token::configDelimiters()); }
	void TearDown() { delete _token; }
};

TEST_F(TokenConfigTest, Basic) {
	std::string testStr(" \nserver { \n\tlisten 8080; \n\tserver_name localhost; \n\troot /var/www/html; \n\tindex index.html; \n\tclient_max_body_size 1M; \n \n\terror_page 404 /404.html; \n \n\tlocation / { \n\t\tallowed_methods GET POST; \n\t\tautoindex on; \n\t} \n \n\tlocation /upload { \n\t\tallowed_methods POST DELETE; \n\t\tupload_path /var/www/uploads; \n\t} \n \n\tlocation .php { \n\t\tcgi_pass /usr/bin/php-cgi; \n\t} \n}");
	
	std::string expectedTokens[] = {"server", "{", "listen", "8080", ";", "server_name", "localhost", ";", "root", "/var/www/html", ";", "index", "index.html", ";", "client_max_body_size", "1M", ";", "error_page", "404", "/404.html", ";", "location", "{", "allowed_methods", "GET", "POST", ";", "autoindex", "on", ";", "}", "location", "/upload", "{", "allowed_methods", "POST", "DELETE", ";", "upload_path", "/var/www/uploads", ";", "}", "location", ".php", "{", "cgi_pass", "/usr/bin/php-cgi", ";", "}", "}"};
	
	int i = 0;
	const char* inputStr = testStr.c_str();
	while(*inputStr) {
		const char* curStr = inputStr;
		inputStr = _token->next(inputStr);
		if (!inputStr)
			break;

		std::string expToken = expectedTokens[i];
		std::string curToken(_token->_start, _token->_len);

		EXPECT_EQ(_token->_len, expToken.length());
		ASSERT_STREQ(curToken.c_str(), expToken.c_str())
			<< PURPLE << "\t|received: \"" << curToken << "\"\t|len: " << expToken.length() << RESET
			<< ORANGE << "\n\t|Expected: \"" << expToken << "\"\t|len: " << curToken.length() << RESET
			<< PURPLE << "\n\n\t|cur input str: " << curStr << RESET;
		i++;
	}
	ASSERT_EQ(i, expectedTokens->length());
}

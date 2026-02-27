// Tester
#include <gtest/gtest.h>
// Other classes in the project
#include "ConfParser.hpp"
#include "Server.hpp"
#include "debug.hpp"
// Library imports
#include <string>
#include <vector>
#include <stdexcept>

#define RESET   "\033[0m"
#define GREEN   "\033[1;32m"
#define RED     "\033[1;31m"

class ConfParserTest: public ::testing::Test {
protected:
	std::vector<Server*> _servers;

	void TearDown() {
		for (size_t i = 0; i < _servers.size(); i++)
			delete _servers[i];
		_servers.clear();
	}

	void parse(const std::string& config) {
		ConfParser parser(config, _servers);
	}

	std::string strv(const StrView& server) {
		return std::string(server.getStart(), server.getLen());
	}
};


TEST_F(ConfParserTest, EmptyServer) {
	parse("server { }");

	ASSERT_EQ(_servers.size(), 1);
	EXPECT_EQ(_servers[0]->getListenLen(), 0);
	EXPECT_EQ(_servers[0]->getLoncationsLen(), 0);
}

TEST_F(ConfParserTest, BasicServer) {
	parse("server { listen 127 8080; root /var/www; }");

	ASSERT_EQ(_servers.size(), 1);
	ASSERT_EQ(_servers[0]->getListenLen(), 1);
	EXPECT_EQ(_servers[0]->_listen[0]._host, 127);
	EXPECT_EQ(_servers[0]->_listen[0]._port, 8080);
	EXPECT_EQ(strv(_servers[0]->_defaults._root), "/var/www");
}

TEST_F(ConfParserTest, ServerWithIndexAndAutoindex) {
	parse(
		"server {\n"
		"  index index.html index.htm default.html;\n"
		"  autoindexing on;\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	EXPECT_EQ(_servers[0]->_defaults._index.size(), 3);
	EXPECT_EQ(strv(_servers[0]->_defaults._index[0]), "index.html");
	EXPECT_EQ(strv(_servers[0]->_defaults._index[1]), "index.htm");
	EXPECT_EQ(strv(_servers[0]->_defaults._index[2]), "default.html");
	EXPECT_TRUE(_servers[0]->_defaults._autoindex);
}

TEST_F(ConfParserTest, ClientMaxBodySize) {
	parse(
		"server { client_max_body_size 1024; }\n"
		"server { client_max_body_size 5k; }\n"
		"server { client_max_body_size 10m; }\n"
		"server { client_max_body_size 2g; }"
	);

	ASSERT_EQ(_servers.size(), 4);
	EXPECT_EQ(_servers[0]->_defaults._clientMaxBody, 1024);
	EXPECT_EQ(_servers[1]->_defaults._clientMaxBody, 5 * 1024);
	EXPECT_EQ(_servers[2]->_defaults._clientMaxBody, 10 * 1024 * 1024);
	EXPECT_EQ(_servers[3]->_defaults._clientMaxBody, 2UL * 1024 * 1024 * 1024);
}

TEST_F(ConfParserTest, ErrorPages) {
	parse(
		"server {\n"
		"  error_page 404 /404.html;\n"
		"  error_page 500 /500.html;\n"
		"  error_page 403 /forbidden.html;\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	EXPECT_EQ(_servers[0]->_defaults._error.size(), 3);
	EXPECT_EQ(strv(_servers[0]->_defaults._error[404]), "/404.html");
	EXPECT_EQ(strv(_servers[0]->_defaults._error[500]), "/500.html");
	EXPECT_EQ(strv(_servers[0]->_defaults._error[403]), "/forbidden.html");
}

TEST_F(ConfParserTest, LocationBasic) {
	parse(
		"server {\n"
		"  location / { }\n"
		"  location /api { }\n"
		"  location /upload { }\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	ASSERT_EQ(_servers[0]->_locations.size(), 3);
	EXPECT_EQ(strv(_servers[0]->_locations[0]._path), "/");
	EXPECT_EQ(strv(_servers[0]->_locations[1]._path), "/api");
	EXPECT_EQ(strv(_servers[0]->_locations[2]._path), "/upload");
}

TEST_F(ConfParserTest, LocationAllowedMethods) {
	parse(
		"server {\n"
		"  location / { allowed_methods GET POST; }\n"
		"  location /api { allowed_methods GET POST PUT DELETE; }\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	ASSERT_EQ(_servers[0]->_locations.size(), 2);

	// Location 0: GET, POST
	unsigned char m0 = _servers[0]->_locations[0]._allowedMethods;
	EXPECT_TRUE(m0 & (1 << 1));   // GET
	EXPECT_TRUE(m0 & (1 << 2));   // POST
	EXPECT_FALSE(m0 & (1 << 3));  // PUT
	EXPECT_FALSE(m0 & (1 << 4));  // DELETE

	// Location 1: GET, POST, PUT, DELETE
	unsigned char m1 = _servers[0]->_locations[1]._allowedMethods;
	EXPECT_TRUE(m1 & (1 << 1));   // GET
	EXPECT_TRUE(m1 & (1 << 2));   // POST
	EXPECT_TRUE(m1 & (1 << 3));   // PUT
	EXPECT_TRUE(m1 & (1 << 4));   // DELETE
}

TEST_F(ConfParserTest, LocationReturn) {
	parse(
		"server {\n"
		"  location /old { return 301 /new; }\n"
		"  location /gone { return 410 /error; }\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	ASSERT_EQ(_servers[0]->_locations.size(), 2);
	EXPECT_EQ(_servers[0]->_locations[0]._returnCode, 301);
	EXPECT_EQ(strv(_servers[0]->_locations[0]._returnPath), "/new");
	EXPECT_EQ(_servers[0]->_locations[1]._returnCode, 410);
	EXPECT_EQ(strv(_servers[0]->_locations[1]._returnPath), "/error");
}

TEST_F(ConfParserTest, LocationUpload) {
	parse(
		"server {\n"
		"  location /upload {\n"
		"    upload_enable on;\n"
		"    upload_path /var/www/uploads;\n"
		"  }\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	ASSERT_EQ(_servers[0]->_locations.size(), 1);
	EXPECT_TRUE(_servers[0]->_locations[0]._uploadEnable);
	EXPECT_EQ(strv(_servers[0]->_locations[0]._uploadPath), "/var/www/uploads");
}

TEST_F(ConfParserTest, LocationCGI) {
	parse(
		"server {\n"
		"  location /cgi-bin {\n"
		"    cgi_extension .py .php .pl;\n"
		"    cgi_path /usr/bin/python3;\n"
		"  }\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);
	ASSERT_EQ(_servers[0]->_locations.size(), 1);
	EXPECT_EQ(_servers[0]->_locations[0]._cgiExtensions.size(), 3);
	EXPECT_EQ(strv(_servers[0]->_locations[0]._cgiExtensions[0]), ".py");
	EXPECT_EQ(strv(_servers[0]->_locations[0]._cgiExtensions[1]), ".php");
	EXPECT_EQ(strv(_servers[0]->_locations[0]._cgiExtensions[2]), ".pl");
	EXPECT_EQ(strv(_servers[0]->_locations[0]._cgiPath), "/usr/bin/python3");
}

TEST_F(ConfParserTest, LocationOverrides) {
	parse(
		"server {\n"
		"  root /var/www/html;\n"
		"  autoindexing off;\n"
		"  client_max_body_size 1m;\n"
		"  location /special {\n"
		"    root /var/www/special;\n"
		"    autoindexing on;\n"
		"    client_max_body_size 5m;\n"
		"  }\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 1);

	// Server defaults
	EXPECT_EQ(strv(_servers[0]->_defaults._root), "/var/www/html");
	EXPECT_FALSE(_servers[0]->_defaults._autoindex);
	EXPECT_EQ(_servers[0]->_defaults._clientMaxBody, 1024 * 1024);

	// Location overrides
	ASSERT_EQ(_servers[0]->_locations.size(), 1);
	EXPECT_EQ(strv(_servers[0]->_locations[0]._overrides._root), "/var/www/special");
	EXPECT_TRUE(_servers[0]->_locations[0]._overrides._autoindex);
	EXPECT_EQ(_servers[0]->_locations[0]._overrides._clientMaxBody, 5 * 1024 * 1024);
}

TEST_F(ConfParserTest, CompleteConfiguration) {
	parse(
		"server {\n"
		"  listen 127 8080;\n"
		"  listen 0 8081;\n"
		"  root /var/www/html;\n"
		"  index index.html index.htm;\n"
		"  client_max_body_size 1m;\n"
		"  autoindexing off;\n"
		"  error_page 404 /404.html;\n"
		"  error_page 500 /500.html;\n"
		"\n"
		"  location / {\n"
		"    allowed_methods GET POST;\n"
		"    autoindexing on;\n"
		"  }\n"
		"\n"
		"  location /upload {\n"
		"    allowed_methods POST DELETE;\n"
		"    upload_enable on;\n"
		"    upload_path /var/www/uploads;\n"
		"    client_max_body_size 10m;\n"
		"  }\n"
		"\n"
		"  location /cgi-bin {\n"
		"    cgi_extension .php;\n"
		"    cgi_path /usr/bin/php-cgi;\n"
		"  }\n"
		"}\n"
		"server {\n"
		"  listen 127 9090;\n"
		"  root /var/www/site2;\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 2);

	// Server 0
	EXPECT_EQ(_servers[0]->_listen.size(), 2);
	EXPECT_EQ(_servers[0]->_listen[0]._port, 8080);
	EXPECT_EQ(_servers[0]->_listen[1]._port, 8081);
	EXPECT_EQ(_servers[0]->_locations.size(), 3);
	EXPECT_EQ(_servers[0]->_defaults._error.size(), 2);
	EXPECT_EQ(strv(_servers[0]->_defaults._root), "/var/www/html");

	// Server 1
	EXPECT_EQ(_servers[1]->_listen.size(), 1);
	EXPECT_EQ(_servers[1]->_listen[0]._port, 9090);
	EXPECT_EQ(_servers[1]->_locations.size(), 0);
	EXPECT_EQ(strv(_servers[1]->_defaults._root), "/var/www/site2");
}

// Error Tests
TEST_F(ConfParserTest, MissingOpenBrace) {
	ASSERT_THROW(parse("server listen 8080; }"), std::runtime_error);
}

TEST_F(ConfParserTest, MissingCloseBrace) {
	ASSERT_THROW(parse("server { listen 127 8080;"), std::runtime_error);
}

TEST_F(ConfParserTest, MissingSemicolon) {
	ASSERT_THROW(parse("server { listen 127 8080 }"), std::runtime_error);
}

TEST_F(ConfParserTest, UnknownDirective) {
	ASSERT_THROW(parse("server { unknown_directive value; }"), std::runtime_error);
}

TEST_F(ConfParserTest, InvalidPath) {
	ASSERT_THROW(parse("server { root relative/path; }"), std::runtime_error);
}

TEST_F(ConfParserTest, InvalidMethod) {
	ASSERT_THROW(parse("server { location / { allowed_methods INVALID; } }"), std::runtime_error);
}

TEST_F(ConfParserTest, InvalidSize) {
	ASSERT_THROW(parse("server { client_max_body_size 10x; }"), std::runtime_error);
}

TEST_F(ConfParserTest, NegativeNumber) {
	ASSERT_THROW(parse("server { listen 127 -8080; }"), std::runtime_error);
}

TEST_F(ConfParserTest, InvalidOnOff) {
	ASSERT_THROW(parse("server { autoindexing maybe; }"), std::runtime_error);
}

TEST_F(ConfParserTest, EmptyAllowedMethods) {
	ASSERT_THROW(parse("server { location / { allowed_methods; } }"), std::runtime_error);
}

TEST_F(ConfParserTest, UnclosedLocation) {
	ASSERT_THROW(parse("server { location / { root /test; }"), std::runtime_error);
}

// Tester
#include <arpa/inet.h>
#include <cstddef>
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
	std::vector<Server*>	_servers;
	std::string				_config;

	enum MethodBits {
		BIT_GET = 1,
		BIT_POST = 2,
		BIT_PUT = 3,
		BIT_DELETE = 4
	};

	void TearDown() {
		for (size_t i = 0; i < _servers.size(); i++)
			delete _servers[i];
		_servers.clear();
	}

	void parse(std::string config) {
		_config = config;
		ConfParser parser(_config, _servers);
		parser.createServers();
	}

	Server* server(size_t idx = 0) {
		EXPECT_LT(idx, _servers.size());
		return _servers[idx];
	}

	Location& location(size_t serverIdx, size_t locIdx) {
		EXPECT_LT(locIdx, server(serverIdx)->getLoncationsLen());
		return server(serverIdx)->_locations[locIdx];
	}

	void assertSingleServer() {
		ASSERT_EQ(_servers.size(), 1);
	}

	bool hasMethod(const Location& loc, int methodBit) {
		return loc.isAllowedMethod(methodBit) != 0;
	}
};

TEST_F(ConfParserTest, EmptyServer) {
	parse("server { }");

	assertSingleServer();
	EXPECT_EQ(server()->getListenLen(), 0);
	EXPECT_EQ(server()->getLoncationsLen(), 0);
}

TEST_F(ConfParserTest, BasicServer) {
	parse("server { listen 127.0.0.1:8080; root /var/www; }");

	assertSingleServer();
	ASSERT_EQ(server()->getListenLen(), 1);
	EXPECT_EQ(server()->_listen[0].getHost(), inet_addr("127.0.0.1"));
	EXPECT_EQ(server()->_listen[0].getPort(), 8080);
	EXPECT_STREQ(server()->_defaults.getRoot(), "/var/www");
}

TEST_F(ConfParserTest, BasicServerNoHost) {
	parse("server { listen 8080; root /var/www; }");

	assertSingleServer();
	ASSERT_EQ(server()->getListenLen(), 1);
	EXPECT_EQ(server()->_listen[0].getHost(), INADDR_ANY);
	EXPECT_EQ(server()->_listen[0].getPort(), 8080);
	EXPECT_STREQ(server()->_defaults.getRoot(), "/var/www");
}

TEST_F(ConfParserTest, ServerWithIndexAndAutoindex) {
	parse(
		"server {\n"
		"  index index.html index.htm default.html;\n"
		"  autoindexing on;\n"
		"}"
	);

	assertSingleServer();
	EXPECT_EQ(server()->_defaults.getIndex().len(), 3);
	EXPECT_STREQ(server()->_defaults.getIndex()[0].getStart(), "index.html");
	EXPECT_STREQ(server()->_defaults.getIndex()[1].getStart(), "index.htm");
	EXPECT_STREQ(server()->_defaults.getIndex()[2].getStart(), "default.html");
	EXPECT_TRUE(server()->_defaults.isAutoindexed());
}

TEST_F(ConfParserTest, ClientMaxBodySize) {
	parse(
		"server { client_max_body_size 1024; }\n"
		"server { client_max_body_size 5k; }\n"
		"server { client_max_body_size 10m; }\n"
		"server { client_max_body_size 2g; }"
	);

	ASSERT_EQ(_servers.size(), 4);
	EXPECT_EQ(server(0)->_defaults.getClientMaxBody(), 1024);
	EXPECT_EQ(server(1)->_defaults.getClientMaxBody(), 5 * 1024);
	EXPECT_EQ(server(2)->_defaults.getClientMaxBody(), 10 * 1024 * 1024);
	EXPECT_EQ(server(3)->_defaults.getClientMaxBody(), 2UL * 1024 * 1024 * 1024);
}

TEST_F(ConfParserTest, ErrorPages) {
	parse("server { error_page 404 /404.html; error_page 500 /500.html; error_page 403 /forbidden.html; }");

	assertSingleServer();
	EXPECT_STREQ(server()->_defaults.findErrorFile(404), "/404.html");
	EXPECT_STREQ(server()->_defaults.findErrorFile(500), "/500.html");
	EXPECT_STREQ(server()->_defaults.findErrorFile(403), "/forbidden.html");
}

TEST_F(ConfParserTest, LocationBasic) {
	parse(
		"server {\n"
		"  location / { }\n"
		"  location /api { }\n"
		"  location /upload { }\n"
		"}"
	);

	assertSingleServer();
	ASSERT_EQ(server()->getLoncationsLen(), 3);
	EXPECT_STREQ(location(0, 0).getPath(), "/");
	EXPECT_STREQ(location(0, 1).getPath(), "/api");
	EXPECT_STREQ(location(0, 2).getPath(), "/upload");
}

TEST_F(ConfParserTest, LocationAllowedMethods) {
	parse(
		"server {\n"
		"  location / { allowed_methods GET POST; }\n"
		"  location /api { allowed_methods GET POST PUT DELETE; }\n"
		"}"
	);

	assertSingleServer();
	ASSERT_EQ(server()->getLoncationsLen(), 2);

	EXPECT_TRUE(hasMethod(location(0, 0), BIT_GET));
	EXPECT_TRUE(hasMethod(location(0, 0), BIT_POST));
	EXPECT_FALSE(hasMethod(location(0, 0), BIT_PUT));
	EXPECT_FALSE(hasMethod(location(0, 0), BIT_DELETE));

	EXPECT_TRUE(hasMethod(location(0, 1), BIT_GET));
	EXPECT_TRUE(hasMethod(location(0, 1), BIT_POST));
	EXPECT_TRUE(hasMethod(location(0, 1), BIT_PUT));
	EXPECT_TRUE(hasMethod(location(0, 1), BIT_DELETE));
}

TEST_F(ConfParserTest, LocationReturn) {
	parse(
		"server {\n"
		"  location /old { return 301 /new; }\n"
		"  location /gone { return 410 /error; }\n"
		"}"
	);

	assertSingleServer();
	ASSERT_EQ(server()->getLoncationsLen(), 2);
	EXPECT_EQ(location(0, 0).getReturncode(), 301);
	EXPECT_STREQ(location(0, 0).getReturnPath(), "/new");
	EXPECT_EQ(location(0, 1).getReturncode(), 410);
	EXPECT_STREQ(location(0, 1).getReturnPath(), "/error");
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

	assertSingleServer();
	ASSERT_EQ(server()->getLoncationsLen(), 1);
	EXPECT_TRUE(location(0, 0).getUploadEnabled());
	EXPECT_STREQ(location(0, 0).getUploadPath(), "/var/www/uploads");
}

TEST_F(ConfParserTest, LocationCGI) {
	parse(
		"server {\n"
		"  location /cgi-bin {\n"
		"    cgi_extension .py .php .pl;\n"
		"    cgi_path /usr/bin/python3 /usr/bin/php /usr/bin/ruby;\n"
		"  }\n"
		"}"
	);

	assertSingleServer();
	ASSERT_EQ(server()->getLoncationsLen(), 1);
	EXPECT_EQ(location(0, 0).getCgiExtensions().len(), 3);
	EXPECT_STREQ(location(0, 0).getCgiExtensions()[0].getStart(), ".py");
	EXPECT_STREQ(location(0, 0).getCgiExtensions()[1].getStart(), ".php");
	EXPECT_STREQ(location(0, 0).getCgiExtensions()[2].getStart(), ".pl");
	EXPECT_EQ(location(0, 0).getCgiPath().len(), 3);
	EXPECT_STREQ(location(0, 0).getCgiPath()[0].getStart(), "/usr/bin/python3");
	EXPECT_STREQ(location(0, 0).getCgiPath()[1].getStart(), "/usr/bin/php");
	EXPECT_STREQ(location(0, 0).getCgiPath()[2].getStart(), "/usr/bin/ruby");
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

	assertSingleServer();
	EXPECT_STREQ(server()->_defaults.getRoot(), "/var/www/html");
	EXPECT_FALSE(server()->_defaults.isAutoindexed());
	EXPECT_EQ(server()->_defaults.getClientMaxBody(), 1024 * 1024);

	ASSERT_EQ(server()->getLoncationsLen(), 1);
	EXPECT_STREQ(location(0, 0).getOverrides().getRoot(), "/var/www/special");
	EXPECT_TRUE(location(0, 0).getOverrides().isAutoindexed());
	EXPECT_EQ(location(0, 0).getOverrides().getClientMaxBody(), 5 * 1024 * 1024);
}

TEST_F(ConfParserTest, CompleteConfiguration) {
	parse(
		"server {\n"
		"  listen 127.0.0.1:8080;\n"
		"  listen 8081;\n"
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
		"  listen 127.0.0.1:9090;\n"
		"  root /var/www/site2;\n"
		"}"
	);

	ASSERT_EQ(_servers.size(), 2);

	EXPECT_EQ(server(0)->getListenLen(), 2);
	EXPECT_EQ(server(0)->_listen[0].getPort(), 8080);
	EXPECT_EQ(server(0)->_listen[1].getPort(), 8081);
	EXPECT_EQ(server(0)->getLoncationsLen(), 3);
	EXPECT_EQ(server(0)->_defaults.getErrorMapSize(), 2);
	EXPECT_STREQ(server(0)->_defaults.getRoot(), "/var/www/html");

	EXPECT_EQ(server(1)->getListenLen(), 1);
	EXPECT_EQ(server(1)->_listen[0].getPort(), 9090);
	EXPECT_EQ(server(1)->getLoncationsLen(), 0);
	EXPECT_STREQ(server(1)->_defaults.getRoot(), "/var/www/site2");
}

TEST_F(ConfParserTest, InvalidConfigurations) {
	struct TestCase {
		const char* name;
		const char* config;
	};

	TestCase cases[] = {
		{"MissingOpenBrace", "server listen 8080; }"},
		{"MissingCloseBrace", "server { listen 8080;"},
		{"MissingSemicolon", "server { listen 8080 }"},
		{"UnknownDirective", "server { unknown_directive value; }"},
		{"InvalidPath", "server { root relative/path; }"},
		{"InvalidMethod", "server { location / { allowed_methods INVALID; } }"},
		{"InvalidSize", "server { client_max_body_size 10x; }"},
		{"NegativeNumber", "server { listen -8080; }"},
		{"InvalidOnOff", "server { autoindexing maybe; }"},
		{"EmptyAllowedMethods", "server { location / { allowed_methods; } }"},
		{"UnclosedLocation", "server { location / { root /test; }"}
	};

	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		SCOPED_TRACE(cases[i].name);
		ASSERT_THROW(parse(cases[i].config), std::runtime_error);
	}
}

TEST_F(ConfParserTest, InvalidIPAddresses) {
	const char* invalidIPs[] = {
		"server { listen 999.999.999.999:8080; }",
		"server { listen 192.168.1:8080; }",
		"server { listen 192.168.1.1.1:8080; }",
		"server { listen abc.def.ghi.jkl:8080; }"
	};

	for (size_t i = 0; i < sizeof(invalidIPs) / sizeof(invalidIPs[0]); i++) {
		SCOPED_TRACE(invalidIPs[i]);
		ASSERT_THROW(parse(invalidIPs[i]), std::runtime_error);
	}
}

TEST_F(ConfParserTest, InvalidPorts) {
	const char* invalidPorts[] = {
		"server { listen 127.0.0.1:99999; }",
		"server { listen 127.0.0.1:0; }",
		"server { listen 127.0.0.1:abc; }",
		"server { listen 99999; }"
	};

	for (size_t i = 0; i < sizeof(invalidPorts) / sizeof(invalidPorts[0]); i++) {
		SCOPED_TRACE(invalidPorts[i]);
		ASSERT_THROW(parse(invalidPorts[i]), std::runtime_error);
	}
}

TEST_F(ConfParserTest, ValidIPFormats) {
	const size_t nServers = 4;
	parse(
		"server { listen 192.168.1.1:8080; }\n"
		"server { listen 0.0.0.0:3000; }\n"
		"server { listen *:4000; }\n"
		"server { listen localhost:5000; }"
	);

	const char* expect[] = {
		"192.168.1.1",
		"0.0.0.0",
		"0.0.0.0",
		"127.0.0.1"
	};
	
	ASSERT_EQ(_servers.size(), nServers);
	for (size_t i = 0; i < nServers; i++)
		EXPECT_EQ(server(i)->_listen[0].getHost(), inet_addr(expect[i]));
}

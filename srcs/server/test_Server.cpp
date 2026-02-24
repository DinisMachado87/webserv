// #include <gtest/gtest.h>
// #include "Server.hpp"
// #include "Listening.hpp"
//
// //Testable server exposes the private members for testing
// class TestableServer : public Server {
// public:
// 	using Server::getSocket;
// 	using Server::addSocket;
// 	using Server::deleteSocket;
// 	using Server::epoll_init;
// 	using Server::_fdEpoll;
// 	using Server::_sockets;
//
// 	int getEpollFd() const { return _fdEpoll; }
// 	size_t getSocketCount() const { return _sockets.size(); }
//
// 	TestableServer( struct sockaddr_in& config ):
// 		Server(config) {};
//
// 	~TestableServer() {};
// };
//
// class ServerTest : public ::testing::Test {
// protected:
// 	TestableServer*		server;
// 	struct sockaddr_in	config;
//
// 	void SetUp() {
// 		server = NULL;
//
// 		config.sin_family = AF_INET;
// 		config.sin_addr.s_addr = htonl(INADDR_ANY);
// 		config.sin_port = htons(8080);
// 	}
//
// 	void TearDown() {
// 		if (server) {
// 			delete server;
// 			server = NULL;
// 		}
// 	}
// };
//
// TEST_F(ServerTest, Basic) {
// 	server = new TestableServer(config);
//
// 	EXPECT_TRUE(server->getEpollFd());
// }

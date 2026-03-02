// test_connection.cpp
#include <gtest/gtest.h>
#include "Listening.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <vector>

class ListeningTest : public ::testing::Test {
protected:
	Server server;
	std::vector<Listening*> connections;

	void SetUp() { connections.clear(); }

	void TearDown() {
		for (std::vector<Listening*>::iterator it = connections.begin(); 
		it != connections.end(); ++it) {
			if (*it) {
				close((*it)->getFd());
				delete *it;
			}
		}
		connections.clear();
	}

	Listen createListenConfig(int port, in_addr_t host = INADDR_ANY) {
		Listen listen;
		listen._host = host;
		listen._port = port;
		return listen;
	}

	Listening* createAndTrackListening(Server& srv, const Listen& config) {
		Listening* lstng = Listening::create(srv, config);
		if (lstng) {
			connections.push_back(lstng);
		}
		return lstng;
	}

	bool isSO_REUSEADDR_Set(int fd) {
		int optval;
		socklen_t optlen = sizeof(optval);
		getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen);
		return optval != 0;
	}

	// Single, simple helper that creates and validates ONE listening
	Listening* createAndValidateListening(Server& srv, int port) {
		Listen listen = createListenConfig(port);
		Listening* lstng = createAndTrackListening(srv, listen);
		
		EXPECT_TRUE(lstng != NULL);
		EXPECT_GT(lstng->getFd(), 0);
		
		return lstng;
	}
};

TEST_F(ListeningTest, CreateSuccessfully) {
	Listening* lstng = createAndValidateListening(server, 8888);
	EXPECT_TRUE(isSO_REUSEADDR_Set(lstng->getFd()));
}

TEST_F(ListeningTest, SO_REUSEADDR_AllowsQuickRestart) {
	Listening* lstng1 = createAndValidateListening(server, 8889);

	close(lstng1->getFd());
	delete lstng1;
	connections.pop_back();

	createAndValidateListening(server, 8889);
}

TEST_F(ListeningTest, MultiplePortsWork) {
	Listening* lstng1 = createAndValidateListening(server, 8890);
	Listening* lstng2 = createAndValidateListening(server, 8891);
	
	EXPECT_NE(lstng1->getFd(), lstng2->getFd());
}

TEST_F(ListeningTest, SocketIsListening) {
	Listening* lstng = createAndValidateListening(server, 8892);

	int type;
	socklen_t len = sizeof(type);
	getsockopt(lstng->getFd(), SOL_SOCKET, SO_TYPE, &type, &len);
	EXPECT_EQ(type, SOCK_STREAM);

	int client = accept(lstng->getFd(), NULL, NULL);
	EXPECT_EQ(client, -1);
	EXPECT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}

TEST_F(ListeningTest, CanAcceptRealListening) {
	createAndValidateListening(server, 8893);
}

TEST_F(ListeningTest, DifferentServersWork) {
	Server server1;
	Server server2;

	createAndValidateListening(server1, 8894);
	createAndValidateListening(server2, 8895);
}

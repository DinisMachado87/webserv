#include "Listening.hpp"

// Public constructors and destructors
Listening::Listening(int fd, int port):
	ASocket(fd),
	_port(port)
{}

Listening::~Listening() {}

// Public Methods
Listening* Listening::create(struct sockaddr_in& sockConfig) {
	const std::string	errMsg = "Error creating Socket: ";

	int	fdNewSocket = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);

	if (0 <= fdNewSocket
		&& OK == bind(fdNewSocket, (struct sockaddr*)&sockConfig, sizeof(sockConfig))
		&& OK == listen(fdNewSocket, BACKLOG_SIZE))
			return new Listening(fdNewSocket, sockConfig.sin_port);

	if (0 < fdNewSocket)
		close(fdNewSocket);
	throw std::runtime_error(errMsg + strerror(errno));
}

void Listening::handle(int fd) {
	(void)fd;
}

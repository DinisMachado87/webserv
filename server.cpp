#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int	main(void)
{
	//create socket
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
	{
		perror("socket");
		return (-1);
	}
	printf("Socket created.\n");


	//set socket to reuse when closed
	int opt = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


	//bind socket to port
	struct sockaddr_in servInfo = {0};
	servInfo.sin_addr.s_addr = 0;
	servInfo.sin_family = AF_INET;
	servInfo.sin_port = htons(5556);
	struct sockaddr* castInfo = (struct sockaddr*)&servInfo;
	socklen_t	addrLen = sizeof(servInfo);
	if (bind(sfd, castInfo, addrLen) == -1)
	{
		perror("bind");
		close(sfd);
		return (-1);
	}
	printf("Socket bound.\n");


	//set socket to listen
	if (listen(sfd, 0) == -1)
	{
		perror("listen");
		close(sfd);
		return (-1);
	}
	printf("Socket listening.\n");



	//wait for a connection
	struct sockaddr_in clientInfo = {0};
	socklen_t	clientSize = 0;
	while(1)
	{
		int cfd = accept(sfd, (struct sockaddr*)&clientInfo, &clientSize);
		if (cfd == -1)
		{
			perror("accept");
			close(sfd);
			return (-1);
		}
		printf("Client connection accepted.\n");


		//read incoming GET
		char	buf[1000];
		read(cfd, buf, 999);
		printf("%s\n", buf);

		//set up html page
		int	pfd = open("index.html", O_RDONLY);
		if (pfd == -1)
		{
			perror("open");
			close(sfd);
			return (-1);
		}
		struct stat	pageStat;
		fstat(pfd, &pageStat);
		off_t	pageSize = pageStat.st_size;

		//send HTTP response
		std::string response = "HTTP/1.1 200 OK\r\n\r\n";
		// write(sfd, response.c_str(), response.length());
		int sent = send(cfd, response.c_str(), response.length(), 0);
		sent = sendfile(cfd, pfd, NULL, pageSize);
		if (sent == -1)
		{
			perror("send");
			close(sfd);
			close(cfd);
			close(pfd);
			return (-1);
		}
		printf("Response sent.\n");
		close(cfd);
		close(pfd);
	}
	close(sfd);
}

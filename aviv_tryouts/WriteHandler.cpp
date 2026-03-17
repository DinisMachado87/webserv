#include "WriteHandler.hpp"
#include "ReadHandler.hpp"
#include "Server.hpp"

#include <unistd.h>
#include <cerrno>
#include <cstdio>

void handle_client_write(std::vector<pollfd>& fds,
                         std::vector<Client>& clients,
                         size_t idx)
{
	Client& c = clients[idx - 1];

	if (c.outSent >= c.out.size())
	{
		if (c.shouldClose)
		{
			close_client(fds, clients, idx);
			return;
		}

		c.out.clear();
		c.outSent = 0;
		c.shouldClose = false;
		c.keepAlive = false;
		c.parser.reset();

		set_client_events(fds, idx, POLLIN);

		if (!c.in.empty())
			process_request(fds, clients, idx);

		return;
	}

	ssize_t n = ::write(fds[idx].fd,
						c.out.data() + c.outSent,
						c.out.size() - c.outSent);

	if (n < 0)
	{
		if (errno == EINTR)
			return;
		perror("write");
		close_client(fds, clients, idx);
		return;
	}

	c.outSent += (size_t)n;

	if (c.outSent >= c.out.size())
	{
		if (c.shouldClose)
		{
			close_client(fds, clients, idx);
			return;
		}

		c.out.clear();
		c.outSent = 0;
		c.shouldClose = false;
		c.keepAlive = false;
		c.parser.reset();

		set_client_events(fds, idx, POLLIN);

		if (!c.in.empty())
			process_request(fds, clients, idx);
	}
}
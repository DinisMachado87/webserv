#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>
#include <vector>
#include <cstddef>

struct Client;

int set_reuseaddr(int fd);
int make_listen_socket(int port);

void close_client(std::vector<pollfd>& fds,
                  std::vector<Client>& clients,
                  size_t idx);

void set_client_events(std::vector<pollfd>& fds, size_t idx, short events);

void accept_client(std::vector<pollfd>& fds,
                   std::vector<Client>& clients);

void handle_client_event(std::vector<pollfd>& fds,
                         std::vector<Client>& clients,
                         size_t idx);

void handle_events(std::vector<pollfd>& fds,
                   std::vector<Client>& clients);

void event_loop(int listen_fd);

#endif
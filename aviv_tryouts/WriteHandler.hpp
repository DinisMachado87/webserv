#ifndef WRITEHANDLER_HPP
#define WRITEHANDLER_HPP

#include <poll.h>
#include <vector>
#include "Client.hpp"

void handle_client_write(std::vector<pollfd>& fds,
                         std::vector<Client>& clients,
                         size_t idx);

#endif
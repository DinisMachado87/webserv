#ifndef READHANDLER_HPP
#define READHANDLER_HPP

#include <poll.h>
#include <vector>
#include "Client.hpp"
#include "Request.hpp"

const char* method_to_string(RequestMethod method);
const char* request_method_to_string(requestMethod method);

void print_http_request_debug(const HttpRequest& req);
void print_request_debug(const Request& req);

void process_request(std::vector<pollfd>& fds,
                     std::vector<Client>& clients,
                     size_t idx);

void handle_client_read(std::vector<pollfd>& fds,
                        std::vector<Client>& clients,
                        size_t idx);

#endif
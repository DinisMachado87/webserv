/* poll_server.cpp (C++98)
   Compile:
   c++ -std=c++98 -Wall -Wextra -Werror poll_server.cpp -o poll_server

   Purpose:
   - Minimal poll() server harness to test your HTTP parser.
   - Splits the big main loop into small functions.

   Behavior:
   - Accepts clients on port 6969
   - Accumulates bytes per client
   - When it sees "\r\n\r\n" (end of headers), it sends a tiny HTML response and closes.

   Notes:
   - This is NOT a full HTTP server.
   - It does not implement keep-alive, partial write scheduling (POLLOUT), or request bodies.
   - It is structured so you can later plug in your incremental parser inside process_request().
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

/* Represents one connected client.
   - fd: client socket
   - in: accumulated input buffer from read() calls

   Later you typically add:
   - parser instance
   - response buffer/state
   - timestamps/timeouts
*/
struct Client
{
    int         fd;
    std::string in;

    Client() : fd(-1), in() {}
    Client(int f) : fd(f), in() {}
};

/* write_all()
   write() can write fewer bytes than requested.
   This helper retries until all bytes are written or an error occurs.

   Returns:
   0 on success
  -1 on error
*/
static int write_all(int fd, const char* buf, size_t len)
{
    size_t off = 0;

    while (off < len)
    {
        ssize_t w = ::write(fd, buf + off, len - off);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}

/* find_double_crlf()
   Searches for "\r\n\r\n" which marks end of HTTP headers.

   Returns:
   index of first '\r' in the sequence, or std::string::npos if not found
*/
static size_t find_double_crlf(const std::string& s)
{
    for (size_t i = 0; i + 3 < s.size(); ++i)
    {
        if (s[i] == '\r' && s[i + 1] == '\n' && s[i + 2] == '\r' && s[i + 3] == '\n')
            return i;
    }
    return std::string::npos;
}

/* set_reuseaddr()
   Enables SO_REUSEADDR to allow quick restart without "Address already in use".

   Returns:
   0 on success
  -1 on error
*/
static int set_reuseaddr(int fd)
{
    int yes = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const void*>(&yes), sizeof(yes)) < 0)
        return -1;
    return 0;
}

/* make_listen_socket()
   Creates a TCP listening socket on the given port.

   Steps:
   1) socket()
   2) setsockopt(SO_REUSEADDR)
   3) bind()
   4) listen()

   Returns:
   listening fd on success, -1 on error
*/
static int make_listen_socket(int port)
{
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return -1;

    if (set_reuseaddr(s) < 0)
        perror("setsockopt");

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        ::close(s);
        return -1;
    }

    if (::listen(s, SOMAXCONN) < 0)
    {
        ::close(s);
        return -1;
    }

    return s;
}

/* close_client()
   Removes a client from the poll list and client list, and closes the fd.

   Convention:
   - fds[0] is the server socket
   - clients[0] corresponds to fds[1]
   - idx is an index into fds, and must be >= 1
*/
static void close_client(std::vector<pollfd>& fds,
                         std::vector<Client>& clients,
                         size_t idx)
{
    int fd = fds[idx].fd;
    ::close(fd);

    fds.erase(fds.begin() + idx);
    clients.erase(clients.begin() + (idx - 1));
}

/* respond_and_close()
   Sends a minimal HTTP response and then closes the client.

   For simplicity:
   - always 200 OK
   - always closes the connection
*/
static void respond_and_close(std::vector<pollfd>& fds,
                              std::vector<Client>& clients,
                              size_t idx,
                              const char* body)
{
    char header[512];
    std::snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "\r\n",
        (unsigned long)std::strlen(body));

    int fd = fds[idx].fd;

    if (write_all(fd, header, std::strlen(header)) < 0)
        perror("write(header)");

    if (write_all(fd, body, std::strlen(body)) < 0)
        perror("write(body)");

    close_client(fds, clients, idx);
}

/* accept_client()
   Accepts one new connection and adds it to:
   - pollfd list (fds)
   - client state list (clients)
*/
static void accept_client(std::vector<pollfd>& fds,
                          std::vector<Client>& clients)
{
    int listen_fd = fds[0].fd;
    int cfd = ::accept(listen_fd, 0, 0);

    if (cfd < 0)
    {
        perror("accept");
        return;
    }

    pollfd p;
    p.fd = cfd;
    p.events = POLLIN;
    p.revents = 0;

    fds.push_back(p);
    clients.push_back(Client(cfd));

    std::cout << "Accepted client fd=" << cfd << std::endl;
}

/* process_request()
   Decides when we have enough bytes to respond.

   Right now:
   - once "\r\n\r\n" exists in the client buffer, we respond and close.

   This is the exact place where you'll plug in your HTTP parser:
   - parser.feed(new_bytes)
   - if DONE -> build Request -> build Response
   - if ERROR -> send 400/413/etc
*/
static void process_request(std::vector<pollfd>& fds,
                            std::vector<Client>& clients,
                            size_t idx)
{
    Client& c = clients[idx - 1];

    /* Basic protection against huge headers. */
    if (c.in.size() > 8192)
    {
        respond_and_close(fds, clients, idx, "<h1>413 Request Header Too Large</h1>");
        return;
    }

    /* Check end-of-headers marker. */
    size_t hdrEnd = find_double_crlf(c.in);
    if (hdrEnd == std::string::npos)
        return;

    /* Debug: print request line (first line up to CRLF). */
    size_t lineEnd = c.in.find("\r\n");
    if (lineEnd != std::string::npos)
        std::cout << "REQ LINE: " << c.in.substr(0, lineEnd) << std::endl;

    /* Extremely primitive routing example:
       If request contains " /bye " we answer Bye, otherwise Hello.
       Later you will parse method + target properly. */
    const char* body = "<h1>Hello World!</h1>";
    if (c.in.find(" /bye ") != std::string::npos)
        body = "<h1>Bye!</h1>";

    respond_and_close(fds, clients, idx, body);
}

/* handle_client_event()
   Reads available bytes from one client socket and appends to that client's buffer.
   After reading, calls process_request() to check if we can respond.

   idx is an index into fds (>= 1).
*/
static void handle_client_event(std::vector<pollfd>& fds,
                                std::vector<Client>& clients,
                                size_t idx)
{
    /* Handle error/hangup conditions. */
    if (fds[idx].revents & (POLLHUP | POLLERR | POLLNVAL))
    {
        close_client(fds, clients, idx);
        return;
    }

    /* Only handle readable sockets. */
    if (!(fds[idx].revents & POLLIN))
        return;

    char buf[4096];
    ssize_t n = ::read(fds[idx].fd, buf, sizeof(buf));

    if (n < 0)
    {
        if (errno == EINTR)
            return;
        perror("read");
        close_client(fds, clients, idx);
        return;
    }

    if (n == 0)
    {
        /* client closed connection */
        close_client(fds, clients, idx);
        return;
    }

    clients[idx - 1].in.append(buf, (size_t)n);

    process_request(fds, clients, idx);
}

/* handle_events()
   Dispatches events from poll():
   - If server socket (fds[0]) readable -> accept a client
   - If a client socket readable -> read client bytes and process
*/
static void handle_events(std::vector<pollfd>& fds,
                          std::vector<Client>& clients)
{
    /* Accept new connections if available. */
    if (fds[0].revents & POLLIN)
        accept_client(fds, clients);

    /* Process clients.
       Important: fds/clents can shrink during processing (close_client).
       So we use a while-like loop with manual index adjustment. */
    for (size_t i = 1; i < fds.size(); ++i)
    {
        size_t before = fds.size();
        handle_client_event(fds, clients, i);

        /* If the client was closed, fds.size() decreased and the element
           at index i is now the next client, so decrement i to re-check. */
        if (fds.size() < before)
            --i;
    }
}

/* event_loop()
   Owns the pollfd list + client list and runs the poll() loop.

   This keeps main() small and focused.
*/
static void event_loop(int listen_fd)
{
    std::vector<pollfd> fds;
    std::vector<Client> clients;

    pollfd server_pfd;
    server_pfd.fd = listen_fd;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;

    fds.push_back(server_pfd);

    while (true)
    {
        int ready = ::poll(&fds[0], (nfds_t)fds.size(), -1);

        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            return;
        }

        handle_events(fds, clients);
    }
}

/* main()
   Sets up the listening socket and starts the event loop. */
int main()
{
    const int port = 6969;

    int listen_fd = make_listen_socket(port);
    if (listen_fd < 0)
    {
        perror("listen socket");
        return 1;
    }

    std::cout << "Listening on http://127.0.0.1:" << port << std::endl;

    event_loop(listen_fd);

    ::close(listen_fd);
    return 0;
}


#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <csignal>
#define VERBOSE 1
// Unix return code
#define OK 0
#define ERR 1
// Multical returns
#define ONGOING 1
#define DONE 0
// EPOLL Macros
#define MAX_EVENTS 1024
#define RESPONSES_CUE_SIZE 10
#define TIMEOUT 1000
#define RECV_SIZE 1000
#define CHUNK_SIZE 1000

// HTTP Parser Limits
#define MAX_HEADER_SIZE 8192
#define MAX_CONTENT_LENGTH 1024

typedef unsigned int uint;
typedef unsigned char uchar;

// volatile sig_atomic_t g_shutdown = 0;

#endif

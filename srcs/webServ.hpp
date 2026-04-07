#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <csignal>
#define VERBOSE 1
// Unix return code
#define OK 0
#define ERR -1
// Multical returns
#define ONGOING false
#define DONE true
// EPOLL Macros
#define MAX_EVENTS 1024
#define RESPONSES_CUE_SIZE 10
#define TIMEOUT 1000
#define RECV_SIZE 1000
#define CHUNK_SIZE 1000
// logger
#define LOGLEVEL LOG
#define LOGGING true
#define LOGTOCLI true
#define LOGTOFILE true

// HTTP Parser Limits
#define MAX_HEADER_SIZE 8192
#define MAX_CONTENT_LENGTH 1024

typedef unsigned int uint;
typedef unsigned char uchar;

extern volatile sig_atomic_t g_shutdown;

#endif

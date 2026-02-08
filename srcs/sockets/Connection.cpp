#include "Connection.hpp"
#include "ASocket.hpp"

// Public constructors and destructors
Connection::Connection(uint32_t fd): ASocket(fd) {}

Connection::~Connection() {}

// Public Methods
void Connection::Handle() {}

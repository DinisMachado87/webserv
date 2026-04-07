#include "Logger.hpp"
#include "webServ.hpp"
#include <csignal>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using std::ifstream;
using std::runtime_error;
using std::string;
using std::stringstream;

runtime_error runtimeErrno(const int errNumber, const char *errMsg) {
	string errorString = errMsg;
	errorString.append(strerror(errNumber));
	return runtime_error(errorString);
}

void signal_handler(int signum) {
	const char msgSigint[] = "\nReceived SIGINT (Ctrl+C). Shutting down.\n";
	const char msgSigterm[] = "\nReceived SIGTERM. Shutting down.\n";
	const char msgDefault[] = "\nReceived signal. Shutting down.\n";
	const char *msg;

	if (signum == SIGINT)
		msg = msgSigint;
	else if (signum == SIGTERM)
		msg = msgSigterm;
	else
		msg = msgDefault;

	write(STDOUT_FILENO, msg, strlen(msg));
	g_shutdown = 1;
}

void setup_signals() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	// Custom handler for graceful shutdown
	sa.sa_handler = signal_handler;
	if (ERR == sigaction(SIGINT, &sa, NULL)
		|| ERR == sigaction(SIGTERM, &sa, NULL))
		throw runtimeErrno(errno, "Error setting up signal");

	// Ignore broken pipe / client is gone
	sa.sa_handler = SIG_IGN;
	if (ERR == sigaction(SIGPIPE, &sa, NULL))
		throw runtimeErrno(errno, "Error setting up signal");
}

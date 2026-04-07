#include "Engine.hpp"
#include "Logger.hpp"
#include <cerrno>
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

static string readFile(const char *filepath) {
	ifstream file(filepath);

	if (!file.is_open())
		throw("Error opening file: ");

	stringstream buffer;
	buffer << file.rdbuf();

	file.close();
	return buffer.str();
}
//
// void signal_handler(int signum) {
// 	char msgSigint[] = "\nReceived SIGINT (Ctrl+C). Shutting down.\n";
// 	char msgSigterm[] = "\nReceived SIGTERM. Shutting down.\n";
// 	char msgDefault[] = "\nReceived signal. Shutting down.\n";
// 	const char *msg;
//
// 	if (signum == SIGINT)
// 		msg = msgSigint;
// 	if (signum == SIGTERM)
// 		msg = msgSigterm;
// 	else
// 		msg = msgDefault;
//
// 	write(STDOUT_FILENO, msg, strlen(msg));
// 	g_shutdown = 1;
// }
//
// void setup_signals() {
// 	struct sigaction sa;
//
// 	sigemptyset(&sa.sa_mask);
// 	sa.sa_flags = 0;
//
// 	// Custom handler for graceful shutdown
// 	sa.sa_handler = signal_handler;
// 	sigaction(SIGINT, &sa, NULL);
// 	sigaction(SIGTERM, &sa, NULL);
//
// 	// Ignore broken pipe / client is gone
// 	sa.sa_handler = SIG_IGN;
// 	sigaction(SIGPIPE, &sa, NULL);
// }
//

int main(int argc, char **argv) {
	try {
		if (argc < 2)
			throw runtime_error(
				"Missing Argument. Use: ./webserv <config/path>");
		else if (argc > 3)
			throw runtime_error(
				"Too many arguments. Use: ./webserv <config/path>");

		const char *configPath = argv[1];
		string config = readFile(configPath);
		Engine engine;
		engine.run(config);
		Logger::deleteLogger();
	} catch (runtime_error err) {
		LOG_ERROR(err);
		return (1);
	}
	return (0);
}

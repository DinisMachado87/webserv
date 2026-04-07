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

volatile sig_atomic_t g_shutdown = 0;

static string readFile(const char *filepath) {
	ifstream file(filepath);

	if (!file.is_open())
		throw("Error opening file: ");

	stringstream buffer;
	buffer << file.rdbuf();

	file.close();
	return buffer.str();
}

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

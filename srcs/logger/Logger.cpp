#include "Logger.hpp"
#include "Server.hpp"
#include <arpa/inet.h>
#include <climits>
#include <cstddef>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

using std::cerr;
using std::flush;

#define COLOR_DEBUG "\033[36m" // Cyan
#define COLOR_INFO "\033[32m"  // Green
#define COLOR_WARN "\033[33m"  // Yellow
#define COLOR_ERROR "\033[31m" // Red
#define COLOR_PURPLE "\033[35m"
#define COLOR_RESET "\033[0m"
#define SEPARATOR " | "

using std::cout;
using std::endl;
using std::runtime_error;
using std::string;
using std::stringstream;

Logger *Logger::_loggerPtr = NULL;
const char *Logger::_labels[]
	= {"[NONE]", "[ERROR]", "[WARNING]", "[DEBUG]", "[LOG]", "[CONTENT]"};
// Public constructors and destructors
Logger::Logger() :
	_level(LOGLEVEL) {}

Logger::~Logger() { _logFile.close(); }

void Logger::deleteLogger() {
	delete _loggerPtr;
	_loggerPtr = NULL;
}

// Public Methods
Logger *Logger::logger() {
	if (!_loggerPtr) {
		_loggerPtr = new Logger;
		if (!_loggerPtr)
			throw runtime_error("Error creating logger instance");

		_loggerPtr->_logFile.open("serverLog.txt", std::iostream::app);
		if (!_loggerPtr->_logFile.is_open())
			throw runtime_error("Error opening logfile");
	}
	return _loggerPtr;
}

void Logger::print(const int level, stringstream &stream) {
	stream << COLOR_RESET << endl;
	string str = stream.str();
	if (LOGTOCLI) {
		if (level == ERROR)
			cerr << str;
		else
			cout << str;
	}
	if (LOGTOFILE)
		_logFile << str;
}

void Logger::color(const int level, stringstream &stream) {
	switch (level) {
	case ERROR:
		stream << COLOR_ERROR;
		break;
	case WARNING:
		stream << COLOR_WARN;
		break;
	case LOG:
		stream << COLOR_INFO;
	default:
		return;
	}
}

void Logger::addHost(stringstream &stream, in_addr_t host) {
	stream << " | Host: ";
	uchar *octet = (uchar *)&host;
	stream << (int)octet[0] << '.' << (int)octet[1] << '.' << (int)octet[2]
		   << '.' << (int)octet[3];
}

void Logger::logError(runtime_error errorMsg) {
	if (ERROR > _level)
		return;
	log(ERROR, errorMsg.what(), NONUM, 0, INT_MAX);
}

void Logger::log(const int level, const char *msg, const int num,
				 const int socket, in_addr_t host) {
	if (level > _level)
		return;

	stringstream stream;
	color(level, stream);
	stream << _clock.nowTime() << SEPARATOR;
	stream << _labels[level];
	if (socket)
		stream << " | Socket: " << socket;
	if (host != INT_MAX)
		addHost(stream, host);
	stream << SEPARATOR << msg;
	if (num != -2)
		stream << num;
	print(level, stream);
}

void Logger::logTitle(const char *msg) {
	if (LOG > _level)
		return;
	stringstream stream;
	stream << COLOR_PURPLE;
	stream << "=====" << msg << "=====";
	print(LOG, stream);
}

void Logger::logServer(const char *msg, const Server &server) {
	if (LOG > _level)
		return;
	stringstream stream;
	stream << msg << '\n';
	server.getServerStr(stream);
	print(LOG, stream);
}

#include "Clock.hpp"
#include <ctime>
#include <sstream>
#include <string>

using std::string;
using std::stringstream;

// Public constructors and destructors
Clock::Clock() {}
Clock::~Clock() {}

// Public Methods
string Clock::now() {
	_unixNow = time(NULL);
	_now = localtime(&_unixNow);
	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", _now);
	return string(buffer);
}

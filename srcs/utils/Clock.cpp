#include "Clock.hpp"
#include <ctime>
#include <sstream>
#include <string>

using std::strftime;
using std::string;
using std::stringstream;

// Public constructors and destructors
Clock::Clock() {}
Clock::~Clock() {}

string Clock::getFormatedTime(const int format) {
	_unixNow = time(NULL);
	_now = localtime(&_unixNow);
	char buffer[80];

	switch (format) {
	case DATE:
		strftime(buffer, sizeof(buffer), "%Y-%m-%d", _now);
		break;
	case TIME:
		strftime(buffer, sizeof(buffer), "%H:%M:%S", _now);
		break;
	case DATETIME:
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", _now);
	}
	return string(buffer);
};
// Public Methods
string Clock::nowTime() { return getFormatedTime(TIME); }
string Clock::nowDate() { return getFormatedTime(DATE); }
string Clock::nowDateTime() { return getFormatedTime(DATETIME); }

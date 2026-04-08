#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <ctime>
#include <string>

class Clock {
private:
	enum e_format { TIME, DATE, DATETIME };
	time_t _unixNow;
	struct tm *_now;
	Clock &operator=(const Clock &other);
	Clock(const Clock &other);

public:
	// Constructors and destructors
	Clock();
	~Clock();

	// Methods
	std::string getFormatedTime(const int format);
	std::string nowDateTime();
	std::string nowDate();
	std::string nowTime();
};

#endif

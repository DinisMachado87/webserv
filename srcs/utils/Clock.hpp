#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <ctime>
#include <string>

class Clock {
private:
	time_t _unixNow;
	struct tm *_now;
	Clock &operator=(const Clock &other);
	Clock(const Clock &other);

public:
	// Constructors and destructors
	Clock();
	~Clock();

	// Methods
	std::string now();
};

#endif

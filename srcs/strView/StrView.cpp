#include "StrView.hpp"
#include <unistd.h>

// Public constructors and destructors
StrView::StrView() {}

StrView::StrView(const StrView& other) {
	(void)other;
}

StrView::~StrView() {}

StrView& StrView::operator=(const StrView& other) {
	if (this == &other) return *this;
	return *this;
}

const char*		StrView::getStart() const { return _start; };
unsigned char	StrView::getLen() const { return _len; };

void			StrView::setStart(const char* start) { _start = start; }
void			StrView::setLen(unsigned char len) { _len = len; }

// Public Methods
void StrView::printToken() const { write(1, &_start, _len); }

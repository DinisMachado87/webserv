#include "StrView.hpp"
#include <string>
#include <unistd.h>

// Public constructors and destructors
StrView::StrView(std::string* buffer, const int offset, const unsigned char len) :
	_rawBuffer(buffer),
	_offset(offset),
	_len(len) {}

StrView::StrView(std::string* buffer) :
	_rawBuffer(buffer),
	_offset(0),
	_len(0) {}

StrView::StrView() :
	_rawBuffer(NULL),
	_offset(0),
	_len(0) {}

StrView::StrView(const StrView& other):
	_rawBuffer(other._rawBuffer),
	_offset(other._offset),
	_len(other._len) {}

StrView::~StrView() {}

StrView& StrView::operator=(const StrView& other) {
	if (this == &other) return *this;

	this->_rawBuffer = other._rawBuffer;
	this->_offset = other._offset;
	this->_len = other._len;

	return *this;
}

const char*		StrView::getStart() const { return _rawBuffer->c_str() + _offset; };
unsigned int	StrView::getLen() const { return _len; };
std::string		StrView::getStr() const { return std::string(getStart(), _len); }
void			StrView::setStart(const char* start) { _offset = start - _rawBuffer->c_str(); }
void			StrView::setLen(unsigned char len) { _len = len; }

// Public Methods
void StrView::printToken() const { write(1, getStart(), _len); }

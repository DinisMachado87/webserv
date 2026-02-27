#include "webServ.hpp"
#include "StrView.hpp"
#include <cstring>
#include <string>
#include <unistd.h>

// Public constructors and destructors
StrView::StrView(std::string& buffer, const int offset, const unsigned char len) :
	_rawBuffer(buffer),
	_offset(offset),
	_len(len) {}

StrView::StrView(std::string& buffer) :
	_rawBuffer(buffer),
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

void StrView::move(std::string& toBuffer) {
	int	offset = toBuffer.length();
	toBuffer.append(getStart(), _len);
	toBuffer.push_back('\0');
	_rawBuffer = toBuffer;
	_offset = offset;
}

unsigned int	StrView::getOffset() const { return _offset; };
const char*		StrView::getStart() const { return _rawBuffer.c_str() + _offset; };
unsigned int	StrView::getLen() const { return _len; };
std::string		StrView::getStr() const { return std::string(getStart(), _len); }

void	StrView::setBuffer(std::string& newBuffer) { _rawBuffer = newBuffer; }
void	StrView::setStart(const char* start) { _offset = start - _rawBuffer.c_str(); }
void	StrView::setLen(unsigned int len) { _len = len; }

void	StrView::setStartAndLen(const char* start, unsigned int len) {
	_offset = start - _rawBuffer.c_str();
	_len = len;
}

// Public Methods
void StrView::updateOffset(unsigned int increase) { _offset += increase; }
void StrView::printStrV() const { write(1, getStart(), _len); }

bool StrView::compare(StrView &strV) const { return compare(strV.getStart()); }
bool StrView::compare(const char *str) const {
	if (OK == strncmp(getStart(), str, _len)
		&& str[_len] == '\0')
		return true;
	return false;
};

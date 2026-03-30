#include "StrView.hpp"
#include "webServ.hpp"
#include <cstddef>
#include <cstring>
#include <string>
#include <unistd.h>

// Public constructors and destructors
StrView::StrView(std::string &buffer, const int offset, const uchar len) :
	_rawBuffer(&buffer),
	_offset(offset),
	_len(len) {}

StrView::StrView(std::string &buffer) :
	_rawBuffer(&buffer),
	_offset(0),
	_len(0) {}

StrView::StrView(const StrView &other) :
	_rawBuffer(other._rawBuffer),
	_offset(other._offset),
	_len(other._len) {}

StrView::~StrView() {}

// Operators overload
StrView &StrView::operator=(const StrView &other) {
	if (this == &other)
		return *this;

	this->_rawBuffer = other._rawBuffer;
	this->_offset = other._offset;
	this->_len = other._len;

	return *this;
}

bool StrView::operator==(const StrView &other) const {
	if (_len != other._len)
		return false;
	return strncmp(getStart(), other.getStart(), _len) == 0;
}

bool StrView::operator!=(const StrView &other) const {
	return !(*this == other);
}

bool StrView::operator==(const char *str) const { return compare(str); }
bool StrView::operator!=(const char *str) const { return !(*this == str); }

bool StrView::operator<(const StrView &other) const {
	int cmpResult = strncmp(getStart(), other.getStart(),
							_len < other._len ? _len : other._len);
	if (cmpResult != 0)
		return cmpResult < 0;
	return _len < other._len;
}

// Getters
const char *StrView::getStart() const { return _rawBuffer->c_str() + _offset; };
const char *StrView::getEnd() const { return getStart() + (_len - 1); }
std::string StrView::getStr() const { return std::string(getStart(), _len); }
size_t StrView::getBufferSize() const { return _rawBuffer->length(); }
uint StrView::getOffset() const { return _offset; };
uint StrView::getLen() const { return _len; };

// Setters
void StrView::setBuffer(std::string &newBuffer) { _rawBuffer = &newBuffer; }
void StrView::setLen(uint len) { _len = len; }

void StrView::setStart(const char *start) {
	_offset = start - _rawBuffer->c_str();
}

void StrView::setStartAndLen(const char *start, uint len) {
	_offset = start - _rawBuffer->c_str();
	_len = len;
}

// Public Methods
void StrView::updateOffset(uint increase) { _offset += increase; }
void StrView::printStrV() const { write(1, getStart(), _len); }
bool StrView::compare(StrView &strV) const { return compare(strV.getStart()); }
void StrView::nullTerminate() { _rawBuffer[_offset + _len - 1] = '\0'; }
void StrView::trimEnd(const size_t trimSize) {
	_len > trimSize ? _len -= trimSize : 0;
}

bool StrView::compare(const char *str) const {
	if (OK == strncmp(getStart(), str, _len) && str[_len] == '\0')
		return true;
	return false;
};

bool StrView::ncompare(const char *str, size_t len) const {
	if (OK == strncmp(getStart(), str, len))
		return true;
	return false;
};

void StrView::move(std::string &toBuffer) {
	int offset = toBuffer.length();
	toBuffer.append(getStart(), _len);
	toBuffer.push_back('\0');
	_rawBuffer = &toBuffer;
	_offset = offset;
}

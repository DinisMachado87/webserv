#include "StrViewPointer.hpp"
#include <cstring>
#include <string>
#include <unistd.h>

StrViewPointer::StrViewPointer(std::string& buffer, unsigned int offset, unsigned int len) :
	_rawBuffer(&buffer),
	_offset(offset),
	_len(len) {}

StrViewPointer::StrViewPointer(std::string& buffer) :
	_rawBuffer(&buffer),
	_offset(0),
	_len(0) {}

StrViewPointer::StrViewPointer(const StrViewPointer& other) :
	_rawBuffer(other._rawBuffer),
	_offset(other._offset),
	_len(other._len) {}

StrViewPointer::~StrViewPointer() {}

StrViewPointer& StrViewPointer::operator=(const StrViewPointer& other) {
	if (this == &other)
		return *this;

	_rawBuffer = other._rawBuffer;
	_offset = other._offset;
	_len = other._len;

	return *this;
}

void StrViewPointer::move(std::string& toBuffer) {
	unsigned int offset = toBuffer.length();
	toBuffer.append(getStart(), _len);
	toBuffer.push_back('\0');
	_rawBuffer = &toBuffer;
	_offset = offset;
}

unsigned int StrViewPointer::getOffset() const {
	return _offset;
}

const char* StrViewPointer::getStart() const {
	return _rawBuffer->c_str() + _offset;
}

unsigned int StrViewPointer::getLen() const {
	return _len;
}

std::string StrViewPointer::getStr() const {
	return std::string(getStart(), _len);
}

void StrViewPointer::setBuffer(std::string& newBuffer) {
	_rawBuffer = &newBuffer;
}

void StrViewPointer::setStart(const char* start) {
	_offset = static_cast<unsigned int>(start - _rawBuffer->c_str());
}

void StrViewPointer::setLen(unsigned int len) {
	_len = len;
}

void StrViewPointer::setStartAndLen(const char* start, unsigned int len) {
	_offset = static_cast<unsigned int>(start - _rawBuffer->c_str());
	_len = len;
}

void StrViewPointer::updateOffset(unsigned int increase) {
	_offset += increase;
}

void StrViewPointer::printStrV() const {
	write(1, getStart(), _len);
}

bool StrViewPointer::compare(StrViewPointer& strV) const {
	return compare(strV.getStart());
}

bool StrViewPointer::compare(const char* str) const {
	if (0 == strncmp(getStart(), str, _len)
		&& str[_len] == '\0')
		return true;
	return false;
}
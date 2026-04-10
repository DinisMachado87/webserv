#ifndef SPAN_H
#define SPAN_H

#include "webServ.hpp"
#include <stdexcept>
#include <vector>

template <typename T> class Span {
private:
	std::vector<T> *_vecBuf;
	uint _offset;
	uchar _len;

public:
	Span(std::vector<T> &vecBuf) :
		_vecBuf(&vecBuf),
		_offset(0),
		_len(0) {}

	Span(std::vector<T> &vecBuf, uint offset, uchar len) :
		_vecBuf(&vecBuf),
		_offset(offset),
		_len(len) {}

	Span(const Span &other) :
		_vecBuf(other._vecBuf),
		_offset(other._offset),
		_len(other._len) {}

	~Span() {}

	// Non-const version calls const version
	Span &operator=(const Span &other) {
		if (this == &other)
			return *this;

		_vecBuf = other._vecBuf;
		_offset = other._offset;
		_len = other._len;
		return *this;
	}

	T &operator[](uint i) {
		return const_cast<T &>(static_cast<const Span &>(*this)[i]);
	}

	// Const version has the actual logic
	const T &operator[](uint i) const {
		if (i >= _len)
			throw std::out_of_range("Span index out of range");
		return (*_vecBuf)[_offset + i];
	}

	size_t len() const { return _len; }
};

#endif

#ifndef SPAN_H
#define SPAN_H

#include <stdexcept>
#include <vector>

template<typename T>
class Span {
private:
	std::vector<T>&		_vecBuf;
	unsigned int		_offset;
	unsigned char		_len;
	
	
public:
	Span(std::vector<T>& vecBuf):
		_vecBuf(vecBuf),
		_offset(0),
		_len(0) {}

	Span(std::vector<T>& vecBuf, unsigned int offset, unsigned char len):
		_vecBuf(vecBuf),
		_offset(offset),
		_len(len) {}
	
	Span(const Span& other):
		_vecBuf(other._vecBuf),
		_offset(other._offset),
		_len(other._len) {}
	
	~Span() {}
	
	// Non-const version calls const version
	Span& operator=(const Span& other) {
		if (this == &other) return *this;

		_offset = other._offset;
		_len = other._len;
		return *this;
	}

	T& operator[](unsigned int i) {
		return const_cast<T&>(
			static_cast<const Span&>(*this)[i]
		);
	}
	
	// Const version has the actual logic
	const T& operator[](unsigned int i) const {
		if (i >= _len)
			throw std::out_of_range("Span index out of range");
		return _vecBuf[_offset + i];
	}
	
	unsigned char len() const { return _len; }
};

#endif

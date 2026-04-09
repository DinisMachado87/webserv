#ifndef STRVIEW_HPP
#define STRVIEW_HPP

#include "webServ.hpp"
#include <string>

class StrView {
private:
	std::string *_rawBuffer;
	uint _offset;
	uint _len;

public:
	// Constructors and destructors
	StrView(std::string &buffer, const int offset, const uchar len);
	StrView(std::string &buffer);
	StrView(const StrView &other);
	~StrView();
	// Operators overload
	StrView &operator=(const StrView &other);
	bool operator==(const StrView &other) const;
	bool operator!=(const StrView &other) const;
	bool operator!=(const char *str) const;
	bool operator==(const char *str) const;
	bool operator<(const StrView &other) const;
	// Getters
	const char *getStart() const;
	std::string getStr() const;
	uint getOffset() const;
	uint getLen() const;
	// Setters
	void setBuffer(std::string &newBuffer);
	void setStart(const char *start);
	void setLen(uint len);
	void setStartAndLen(const char *start, uint len);
	// Methods
	void streamBuffer(std::stringstream &stream) const;
	void streamStrView(std::stringstream &stream);
	bool ncompare(const char *str, size_t len) const;
	size_t getBufferSize() const;
	void nullTerminate();
	const char *getEnd() const;
	void trimEnd(const size_t trimSize);
	bool compare(StrView &str) const;
	bool compare(const char *str) const;
	void updateOffset(uint increase);
	void printStrV() const;
	void move(std::string &toBuffer);
};

// Out of class operators for reverse comparison
inline bool operator==(const char *str, const StrView &sv) { return sv == str; }
inline bool operator!=(const char *str, const StrView &sv) { return sv != str; }

#endif

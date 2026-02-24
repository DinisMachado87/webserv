#ifndef STRVIEW_HPP
#define STRVIEW_HPP

#include <string>
class StrView {
private:
	std::string*	_rawBuffer;
	unsigned int	_offset;
	unsigned int	_len;

public:
	// Constructors and destructors
	StrView(std::string* buffer, const int offset, const unsigned char len);
	StrView(std::string* buffer);
	StrView(const StrView& other);
	StrView();
	~StrView();

	// Operators overload
	StrView& operator=(const StrView& other);

	// Getters and setters
	const char*		getStart() const;
	unsigned int	getLen() const;
	std::string		getStr() const;
	void			setStart(const char* start);
	void			setLen(unsigned char len);

	// Methods
	const char*		_start() const;
	void			printToken() const;

};

#endif


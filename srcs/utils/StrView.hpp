#ifndef STRVIEW_HPP
#define STRVIEW_HPP

#include <string>
class StrView {
private:
	std::string&	_rawBuffer;
	unsigned int	_offset;
	unsigned int	_len;

public:
	// Constructors and destructors
	StrView(std::string& buffer, const int offset, const unsigned char len);
	StrView(std::string& buffer);
	StrView(const StrView& other);
	~StrView();

	// Operators overload
	StrView& operator=(const StrView& other);

	// Getters and setters
	const char*		getStart() const;
	unsigned int	getLen() const;
	std::string		getStr() const;

	void			setBuffer(std::string& newBuffer);
	void			setStart(const char* start);
	void			setLen(unsigned int len);
	void			setStartAndLen(const char* start, unsigned int len);

	// Methods
	bool			compare(StrView &str) const;
	bool			compare(const char *str) const;
	void			updateOffset(unsigned int increase);
	unsigned int	getOffset() const;
	void			printStrV() const;
	void			move(std::string& toBuffer);

};

#endif

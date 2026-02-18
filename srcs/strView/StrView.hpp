#ifndef STRVIEW_HPP
#define STRVIEW_HPP

class StrView {
private:
	const char*		_start;
	unsigned char	_len;

public:
	// Constructors and destructors
	StrView();
	StrView(const StrView& other);
	~StrView();

	// Operators overload
	StrView& operator=(const StrView& other);

	// Getters and setters
	const char*		getStart() const;
	unsigned char	getLen() const;

	void			setStart(const char* start);
	void			setLen(unsigned char len);

	// Methods
	void printToken() const;

};

#endif


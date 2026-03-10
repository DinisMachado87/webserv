#ifndef STRVIEWPOINTER_HPP
#define STRVIEWPOINTER_HPP

#include <string>

class StrViewPointer {
private:
	std::string*	_rawBuffer;
	unsigned int	_offset;
	unsigned int	_len;

public:
	/* Constructors and destructor */
	StrViewPointer(std::string& buffer, unsigned int offset, unsigned int len);
	StrViewPointer(std::string& buffer);
	StrViewPointer(const StrViewPointer& other);
	~StrViewPointer();

	/* Assignment operator */
	StrViewPointer& operator=(const StrViewPointer& other);

	/* Getters */
	const char*		getStart() const;
	unsigned int	getLen() const;
	unsigned int	getOffset() const;
	std::string		getStr() const;

	/* Setters */
	void			setBuffer(std::string& newBuffer);
	void			setStart(const char* start);
	void			setLen(unsigned int len);
	void			setStartAndLen(const char* start, unsigned int len);

	/* Methods */
	bool			compare(StrViewPointer& str) const;
	bool			compare(const char* str) const;
	void			updateOffset(unsigned int increase);
	void			printStrV() const;
	void			move(std::string& toBuffer);
};

#endif
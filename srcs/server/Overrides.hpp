#ifndef OVERRIDES_HPP
#define OVERRIDES_HPP

#include "Span.hpp"
#include "StrView.hpp"
#include <cstddef>
#include <map>
#include <sstream>

struct Overrides {
	// Constructor
	Overrides(std::string &buffer, std::vector<StrView> &vecBuf);
	// Vars
	std::map<uint, StrView> _error;
	StrView _root;
	bool _autoindex;
	Span<StrView> _index;
	size_t _clientMaxBody;
	// Getters
	size_t getClientMaxBody() const;
	const char *findErrorFile(uint errorCode) const;
	bool isAutoindexed() const;
	const char *getRoot() const;
	const Span<StrView> &getIndex() const;
	size_t getErrorMapSize() const;
	const char *safeStr(const char *str) const;
	void printOverrides(const char *label, std::stringstream &stream) const;
};

#endif

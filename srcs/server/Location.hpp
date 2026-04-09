#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "Overrides.hpp"
#include "Span.hpp"
#include "StrView.hpp"
#include <string>
#include <vector>

struct Location {
	// Construnctor
	Location(std::string &strBuf, std::vector<StrView> &vecBuf,
			 Overrides *serverDefaults);
	// Assignement Operator
	Location &operator=(const Location &other);

	enum _e_allowed_methods { DEFAULT, GET, POST, DELETE };
	static const char *_methodStrs[4];
	// Substructs
	Overrides _overrides;
	// Pointer to server defaults for comparison getters
	Overrides *_serverDefaults;
	// Member vars
	Span<StrView> _cgiExtensions;
	Span<StrView> _cgiPath;
	StrView _path;
	StrView _returnPath;
	StrView _rewrite_old;
	StrView _rewrite_new;
	StrView _uploadPath;
	uint _returnCode;
	bool _uploadEnable;
	uchar _allowedMethods;
	// Getters Location Vars
	uchar isAllowedMethod(uchar methodToCheck) const;
	const char *findCgiPath(StrView &extention) const;
	const char *findCgiPath(const char *extention) const;
	const char *getPath() const;
	const char *getUploadPath() const;
	const char *getRewriteNewPath() const;
	const char *getRewriteOldPath() const;
	const char *getReturnPath() const;
	uint getReturncode() const;
	bool getUploadEnabled() const;
	const Span<StrView> &getCgiExtensions() const;
	const Span<StrView> &getCgiPath() const;
	const Overrides &getOverrides() const;
	const char *safeStr(const char *str) const;
	void printLocation(size_t index, std::stringstream &stream) const;
	void printStrvSpan(const char *msg, const Span<StrView> &span,
					   std::stringstream &stream) const;
};

#endif

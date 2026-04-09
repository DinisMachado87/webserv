#include "Location.hpp"
#include "Overrides.hpp"
#include "Span.hpp"
#include "StrView.hpp"
#include "webServ.hpp"
#include <cstddef>
#include <iostream>
#include <sstream>

using std::size_t;
using std::stringstream;

// Public constructors and destructors

Location::Location(std::string &strBuf, std::vector<StrView> &vecBuf,
				   Overrides *serverDefaults) :
	_overrides(strBuf, vecBuf),
	_serverDefaults(serverDefaults),
	_cgiExtensions(vecBuf),
	_cgiPath(vecBuf),
	_path(strBuf),
	_returnPath(strBuf),
	_rewrite_old(StrView(strBuf)),
	_rewrite_new(StrView(strBuf)),
	_uploadPath(strBuf),
	_returnCode(0),
	_uploadEnable(false),
	_allowedMethods((1 << GET) | (1 << POST) | (1 << DELETE)) {}

Location &Location::operator=(const Location &other) {
	if (this == &other) {
		return *this;
	}
	_overrides = other._overrides;
	_serverDefaults = other._serverDefaults;
	_cgiExtensions = other._cgiExtensions;
	_cgiPath = other._cgiPath;
	_path = other._path;
	_returnPath = other._returnPath;
	_rewrite_old = other._rewrite_old;
	_rewrite_new = other._rewrite_new;
	_uploadPath = other._uploadPath;
	_returnCode = other._returnCode;
	_uploadEnable = other._uploadEnable;
	_allowedMethods = other._allowedMethods;
	return *this;
}

const Overrides &Location::getOverrides() const { return _overrides; }
const char *Location::getPath() const { return _path.getStart(); }
const char *Location::getReturnPath() const { return _returnPath.getStart(); }
const char *Location::getUploadPath() const { return _uploadPath.getStart(); }
uint Location::getReturncode() const { return _returnCode; }
bool Location::getUploadEnabled() const { return _uploadEnable; }

const char *Location::getRewriteOldPath() const {
	return _rewrite_old.getStart();
}

const char *Location::getRewriteNewPath() const {
	return _rewrite_new.getStart();
}

uchar Location::isAllowedMethod(uchar methodToCheck) const {
	return _allowedMethods & (1 << methodToCheck);
};

// CGI
const Span<StrView> &Location::getCgiPath() const { return _cgiPath; }

const Span<StrView> &Location::getCgiExtensions() const {
	return _cgiExtensions;
}

const char *Location::findCgiPath(StrView &extention) const {
	return findCgiPath(extention.getStart());
}

const char *Location::findCgiPath(const char *extention) const {
	for (size_t i = 0; i < _cgiExtensions.len(); i++)
		if (_cgiExtensions[i].compare(extention))
			return _cgiPath[i].getStart();
	return NULL;
}

const char *Location::safeStr(const char *str) const {
	return str ? str : "NULL";
}

void Location::printStrvSpan(const char *msg, const Span<StrView> &span,
							 stringstream &stream) const {
	size_t i = 0;
	stream << msg;
	for (i = 0; i < span.len(); i++)
		stream << safeStr(span[i].getStart()) << ", ";
	if (i == 0)
		stream << "NONE";
	stream << '\n';
}

void Location::printLocation(size_t index, stringstream &stream) const {
	stream << "  [" << index << "] Path: " << safeStr(getPath()) << '\n';
	stream << "\tReturn Code: " << getReturncode() << '\n';
	stream << "\tReturn Path: " << safeStr(getReturnPath()) << '\n';
	stream << "\tUpload Enabled: " << (getUploadEnabled() ? "true" : "false")
		   << '\n';
	stream << "\tUpload Path: " << safeStr(getUploadPath()) << '\n';

	printStrvSpan("\tCGI Extensions: ", _cgiExtensions, stream);
	printStrvSpan("\tCGI Paths: ", _cgiPath, stream);

	stream << "\tAllowed Methods: " << static_cast<int>(_allowedMethods)
		   << '\n';
	stream << "GETCGIPATH .py: " << safeStr(findCgiPath(".py")) << "\n";
}

#include "Overrides.hpp"
#include "Span.hpp"
#include "StrView.hpp"
#include "webServ.hpp"
#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

using std::map;
using std::string;
using std::stringstream;
using std::vector;

Overrides::Overrides(string &strBuf, vector<StrView> &vecBuf) :
	_root(strBuf),
	_autoindex(false),
	_index(vecBuf),
	_clientMaxBody(0) {}

const Span<StrView> &Overrides::getIndex() const { return _index; };
const char *Overrides::getRoot() const { return _root.getStart(); };
bool Overrides::isAutoindexed() const { return _autoindex; };
size_t Overrides::getClientMaxBody() const { return _clientMaxBody; };
size_t Overrides::getErrorMapSize() const { return _error.size(); };

const char *Overrides::findErrorFile(uint errorCode) const {
	std::map<uint, StrView>::const_iterator it = _error.find(errorCode);
	return ((it != _error.end()) ? it->second.getStart() : NULL);
};

const char *Overrides::safeStr(const char *str) const {
	return str ? str : "NULL";
}

void Overrides::printOverrides(const char *label, stringstream &stream) const {
	stream << "\n" << label << ":" << '\n';
	stream << "  Root: " << safeStr(getRoot()) << '\n';
	stream << "  Autoindex: " << (isAutoindexed() ? "true" : "false") << '\n';
	stream << "  Client Max Body: " << getClientMaxBody() << '\n';

	stream << "  Index files: ";
	for (size_t i = 0; i < _index.len(); i++)
		stream << safeStr(_index[i].getStart()) << ", ";
	stream << "\n";

	stream << '\n' << "  Error pages: ";
	map<uint, StrView>::const_iterator curError = _error.begin();
	map<uint, StrView>::const_iterator end = _error.end();
	for (; curError != end; curError++)
		stream << curError->first << ": "
			   << safeStr(curError->second.getStart()) << '\n';
}

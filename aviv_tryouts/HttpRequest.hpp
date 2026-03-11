#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "HeaderField.hpp"
#include <string>
#include <vector>

enum RequestMethod
{
	METHOD_GET,
	METHOD_POST,
	METHOD_DELETE,
	METHOD_UNKNOWN
};

class HttpRequest
{
public:
	RequestMethod				method;
	std::string					methodString;
	std::string					target;
	std::string					path;
	std::string					queryString;
	std::string					version;
	std::vector<HeaderField>	headers;
	std::string					body;
	int							contentLength;

public:
	HttpRequest() :
		method(METHOD_UNKNOWN),
		methodString(),
		target(),
		path(),
		queryString(),
		version(),
		headers(),
		body(),
		contentLength(-1) {}

	void clear()
	{
		method = METHOD_UNKNOWN;
		methodString.clear();
		target.clear();
		path.clear();
		queryString.clear();
		version.clear();
		headers.clear();
		body.clear();
		contentLength = -1;
	}

	std::string getHeader(const std::string& key) const
	{
		std::string needle = toLower(key);

		for (size_t i = 0; i < headers.size(); ++i)
		{
			if (toLower(headers[i].name) == needle)
				return headers[i].value;
		}
		return std::string();
	}

	static std::string toLower(const std::string& s)
	{
		std::string out = s;

		for (size_t i = 0; i < out.size(); ++i)
		{
			if (out[i] >= 'A' && out[i] <= 'Z')
				out[i] = static_cast<char>(out[i] - 'A' + 'a');
		}

		return out;
	}
};

#endif
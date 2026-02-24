#ifndef CONFPARSER_HPP
#define CONFPARSER_HPP

#include "Expect.hpp"
#include "Server.hpp"
#include "Token.hpp"
#include "Span.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

class ConfParser {
private:
	enum e_state { NONE, SERVER, LOCATION, ENDOFILE };
	enum e_block { INIT_LOCATION, ENDLINE, ENDBLOCK, ENDFILE };

	std::vector<Server *>	_servers;
	Server*					_newServer;
	Location				_newLocation;

	const char*			_curStrConfig;
	unsigned int		_vecCursor;
	Token				_token;
	unsigned char		_curType;
	Expect				_expect;

	// Explicit disables
	ConfParser();
	ConfParser& operator=(const ConfParser& other);
	ConfParser(const ConfParser& other);

	// Methods
	std::vector<Server*>	createServers();
	void			nextServer();
	unsigned char	parseServer();
	void			parseServerLine();
	void			parseLocation();
	void			parseLocationParam();
	bool			parseOverrides(Overrides& overrides);
	void			parseMethod();

	// Error handler
	std::runtime_error	parsingErr(const char* expected) const ;

public:
	// Constructors and destructors
	ConfParser(std::string configStr);
	~ConfParser();

	// Methods
	Server*			next();
};

#endif


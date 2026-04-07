

#include "DirectoryResponse.hpp"
#include "ErrorResponse.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>



DirectoryResponse::DirectoryResponse(Location* loc, Request* req) : Response(loc, req)
{

}
DirectoryResponse::~DirectoryResponse(void)
{

}

int	DirectoryResponse::generateHeader(void)
{
	std::stringstream header;
	header << "HTTP/1.1 200 OK\r\n";
	char buf[64];
	getTime(buf,64);
	header << "Date: " << buf << "\r\n";
	header << "Server: " << SERVER_NAME << "\r\n";
	header << "Content-Type: text/html\r\n";
	if (_responseBody.size() > 0)
		header << "Content-Length: " << _responseBody.size() << "\r\n";
	header << "\r\n";
	_responseHeader = header.str();
	return 0;
}

int	DirectoryResponse::setResponseBody(void)
{
	DIR *dir = opendir(_request->getFilePath().c_str());
	if (!dir)
		return -1;

	std::stringstream body;
	body << "<!DOCTYPE html>\r\n";
	body << "<html>\r\n";
	body << "<head>\r\n";
	body << "<title>Index of " << _request->getFilePath() << "</title>\r\n";
	body << "</head>\r\n";
	body << "<body>\r\n";
	body << "<h1>Index of " << _request->getFilePath() << "</h1>\r\n";
	body << "<ul>\r\n";

	struct dirent *entry;
	struct stat st;

	while ((entry = readdir(dir)) != NULL) //iterating over directory entries.
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue;

		std::string fullPath = _request->getFilePath();
		if (fullPath[fullPath.length() - 1] != '/')
			fullPath += "/";
		fullPath += name;

		if (stat(fullPath.c_str(), &st) == -1) // this is one system call per loop, maybe an overkill!!!
			continue;

		bool isDir = S_ISDIR(st.st_mode); // check if the entry is a directory
		std::string href = name;
		if (isDir)
			href += "/";

		body << "<li><a href=\"" << href << "\">" << name;
		if (isDir)
			body << "/";
		body << "</a></li>\r\n";
	}

	closedir(dir);

	body << "</ul>\r\n";
	body << "</body>\r\n";
	body << "</html>\r\n";

	_responseBody = body.str();
	return 0;
}

bool	DirectoryResponse::sendResponse(const int &clientFD)
{
	try {
		setResponseBody();
		generateHeader();
		ssize_t	ret = 0;
		ret = send(clientFD, _responseHeader.c_str(), _responseHeader.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		ret = send(clientFD, _responseBody.c_str(), _responseBody.size(), 0);
		if (ret < 0)
			throw std::runtime_error("sendResponse: send failure");
		std::cout << "Sent to client:\n" << _responseHeader << _responseBody << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		ErrorResponse error(_location, NULL);
		error.setErrorCode(500);
		error.sendResponse(clientFD);
	}
	return 1;
}

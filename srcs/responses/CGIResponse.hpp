/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/19 16:52:53 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"

extern char**	environ;

							//RFC3875
typedef	struct	metaVs	{	//^^ = must be set
char*	AUTH_TYPE;			//mechanism used to authenticate the user [not needed?]
char*	CONTENT_LENGTH;		//length of message body *set only if there is a body* [not needed for GET]
char*	CONTENT_TYPE;		//media type of message body [not needed for GET]
char*	GATEWAY_INTERFACE;	//^^CGI dialect number e.g. CGI/1.1
char*	PATH_INFO;			//^^path after the file name as presented in URL
char*	PATH_TRANSLATED;	//path as it relates to the server system
char*	QUERY_STRING;		//^^information for the CGI script to affect the return value - URL after '?'
char*	REMOTE_ADDR;		//^^network address of client sending the request (ipv4 or ipv6)
char*	REMOTE_HOST;		//^^domain name of the client sending the request, or NULL
char*	REMOTE_IDENT;		//identity information about the client [not needed]
char*	REMOTE_USER;		//user identification supplied by client
char*	REQUEST_METHOD;		//^^GET / POST / PUT / DELETE etc
char*	SCRIPT_NAME;		//^^path and name of the .cgi file to be run
char*	SERVER_NAME;		//^^name or IP address of our server
char*	SERVER_PORT;		//^^port of our server
char*	SERVER_PROTOCOL;	//^^HTTP version e.g. HTTP/1.1, or INCLUDED when being included as part of a composite document
char*	SERVER_SOFTWARE;	//^^Name and version of server software
}	CGImetaVs;


class	CGIResponse : public Response
{
public:
	CGIResponse(Location* loc, reqVariables* vars, std::string* requestBody);
	~CGIResponse(void);
	int				sendResponse(const int& clientFD);
	std::string*	getCGIoutput(void);
	void			setRequestBody(std::string* body);



	private:
	CGImetaVs	_metaVs;

	int		createHeader(void);
	int		runCGI(void);
	int		childProcess(int pipeP2C[2], int pipeC2P[2]);
	void	freeMetaVs(void);
	void	initialiseMetaVs(void);
	void	setEnvironment(void);
	void	splitPaths(std::string &requestPath);


	CGIResponse(const CGIResponse &other);
	CGIResponse &	operator=(const CGIResponse &other);
};



/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/10 14:22:22 by smoon             #+#    #+#             */
/*   Updated: 2026/03/17 16:07:12 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

Response::Response(Location* loc, reqVariables* vars, std::string* requestBody) : _location(loc), _requestVars(vars), _requestBody(requestBody)
{

}
Response::~Response(void)
{

}

void	Response::getTime(char* buf, int bufSize)
{
	std::time_t current = std::time(NULL);
	std::strftime(buf, bufSize, "%a, %d %b %Y %H:%M:%S", std::localtime(&current));
}

void	initialise_everything(Location* loc, reqVariables* vars, Overrides* over, std::string* body)
{
	(void)loc;
	(void)vars;
	(void)over;
	vars->method = POST;
	vars->contentLength = body->size();
	vars->port = 5555;
	vars->requestPath = "includes/cgi-bin/hello.cgi";
	// vars->contentType =
	// vars->requestPath = "teams/users";
	vars->queryString = "query=hi";
	vars->scriptName = "includes/cgi-bin/hello.cgi";
	vars->REMOTE_ADDR = "175.0.0.23";
	vars->REMOTE_HOST = "client.com";
}

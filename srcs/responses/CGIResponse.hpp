/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/30 17:18:04 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <algorithm>
#include "Response.hpp"

extern char**	environ;

enum e_CGIFileType { CGI, PY, PHP };

class	CGIResponse : public Response
{
public:
	CGIResponse(Location* loc, Request* req, const int& port);
	~CGIResponse(void);
	bool			sendResponse(const int& clientFD);
	std::string*	getCGIoutput(void);



	private:
	bool			_headerSent;
	int				_port;
	e_CGIFileType	_CGIFileType;
	int				setResponseBody(void);
	int				childProcess(const int (&pipeP2C)[2], const int (&pipeC2P)[2]);
	int				generateHeader(void);
	void			setEnvironment(void);


	CGIResponse(const CGIResponse &other);
	CGIResponse &	operator=(const CGIResponse &other);
};

std::string	toHex(int val);



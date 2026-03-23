/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/23 14:48:07 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"

extern char**	environ;

class	CGIResponse : public Response
{
public:
	CGIResponse(Location* loc, reqVariables* vars);
	~CGIResponse(void);
	bool			sendResponse(const int& clientFD, const int &port);
	std::string*	getCGIoutput(void);



	private:
	int			_port;

	int		runCGI(void);
	int		childProcess(const int (&pipeP2C)[2], const int (&pipeC2P)[2]);
	int		generateHeader(void);
	void	freeMetaVs(void);
	void	setEnvironment(void);


	CGIResponse(const CGIResponse &other);
	CGIResponse &	operator=(const CGIResponse &other);
};



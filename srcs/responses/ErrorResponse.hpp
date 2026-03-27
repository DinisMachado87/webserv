/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponse.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/27 14:59:12 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"


class	ErrorResponse : public Response
{
public:
	ErrorResponse(Location* loc, Request* req);
	~ErrorResponse(void);



	private:
	uint	_errorCode;

	static const std::map<uint, const char*>	_errorTitles;
	static const std::map<uint, const char*>	_errorBodies;
	static const char	_msg1[152];
	//code and title
	static const char	_msg2[911];
	//code and title
	static const char	_msg3[14];
	//body
	static const char	_msg4[27];
	int			generateHeader(void);
	int			setResponseBody(void);
	static void	initialiseErrors(void);


	ErrorResponse(const ErrorResponse &other);
	ErrorResponse &	operator=(const ErrorResponse &other);
};



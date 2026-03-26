/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/25 17:17:34 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"

class	GetResponse : public Response
{
public:
	GetResponse(Location* loc, Request* req);
	~GetResponse(void);
	// bool			sendResponse(const int& clientFD);



	private:
	int	generateHeader(void);
	int	setResponseBody(void);


	GetResponse(const GetResponse &other);
	GetResponse &	operator=(const GetResponse &other);
};



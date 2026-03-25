/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/23 15:27:47 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"

class	GetResponse : public Response
{
public:
	GetResponse(Location* loc, reqVariables* vars);
	~GetResponse(void);
	bool			sendResponse(const int& clientFD, const int &port);



	private:
	int		generateHeader(void);


	GetResponse(const GetResponse &other);
	GetResponse &	operator=(const GetResponse &other);
};



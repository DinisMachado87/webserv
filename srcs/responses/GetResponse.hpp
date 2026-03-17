/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/17 14:08:37 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"


class	GetResponse : public Response
{
public:
	GetResponse(Location* loc, reqVariables* vars, std::string* requestBody);
	~GetResponse(void);
	int				sendResponse(int clientFD);



	private:
	int		createHeader(void);
	void	setRequestBody(std::string* body);
	int		getResponseBody(void);
	int		_responseStatus;


	GetResponse(const GetResponse &other);
	GetResponse &	operator=(const GetResponse &other);
};



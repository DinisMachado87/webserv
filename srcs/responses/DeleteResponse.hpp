/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DeleteResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/26 13:15:37 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"

class	DeleteResponse : public Response
{
public:
	DeleteResponse(Location* loc, Request* req);
	~DeleteResponse(void);
	// bool	sendResponse(const int& clientFD);



	private:
	int		generateHeader(void);
	int		deleteFile(void);
	bool	sendResponse(const int &clientFD);

	DeleteResponse(const DeleteResponse &other);
	DeleteResponse &	operator=(const DeleteResponse &other);
};



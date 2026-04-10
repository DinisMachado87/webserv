/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PostResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/03/26 13:15:37 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"

class	PostResponse : public Response
{
public:
	PostResponse(Location* loc, Request* req);
	~PostResponse(void);
	// bool	sendResponse(const int& clientFD);



	private:
	int		generateHeader(void);
	int		actionPost(void);
	bool	sendResponse(const int &clientFD);


	PostResponse(const PostResponse &other);
	PostResponse &	operator=(const PostResponse &other);
};



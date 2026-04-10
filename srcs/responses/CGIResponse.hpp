/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIResponse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoon <smoon@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 16:40:12 by smoon             #+#    #+#             */
/*   Updated: 2026/04/10 17:10:17 by smoon            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <algorithm>
#include "Response.hpp"

extern char**	environ;

enum e_CGIFileType { CGI, PY, PHP };

enum e_TransferEncoding { FULL, CHUNK };

enum e_CGIReadState { CHUNK_LEN, CHUNK_BOD, FULL };

class	CGIResponse : public Response
{
public:
	CGIResponse(Location* loc, Request* req, const int& port);
	~CGIResponse(void);
	bool			readBodyFirst(char buffer[], ssize_t bytesRead);
	bool			readBodyLoop(char buffer[], ssize_t bytesRead);
	bool			sendResponse(const int& clientFD);
	std::string*	getCGIoutput(void);



private:
	bool				_headerSent;
	int					_port;
	int					_childPid;
	int					_clientFD;
	ssize_t				_totalRead;
	int					_pipeP2C[2];
	int					_pipeC2P[2];
	char				_readBuffer[RECV_SIZE + 1];
	ssize_t				_readBufEnd;
	ssize_t				_toRead;
	std::string			_requestBody;
	e_CGIFileType		_CGIFileType;
	e_CGIReadState		_readState;
	e_TransferEncoding	_TransferEnc;

	int				setResponseBody(void);
	int				childProcess(void);
	int				generateHeader(void);
	void			setEnvironment(void);
	void			initChild(void);
	bool			readBodyFirstChunked(char buffer[], ssize_t bytesRead);
	bool			readBodyChunked(char buffer[], ssize_t bytesRead, ssize_t& start);
	bool			readBodyFull(char buffer[], ssize_t bytesRead);
	char*			readLenChunked(char buffer[], ssize_t bytesRead, ssize_t& start);


	CGIResponse(const CGIResponse &other);
	CGIResponse &	operator=(const CGIResponse &other);
};

std::string	toHex(int val);



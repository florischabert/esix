/**
 * @file
 * Standard API for UDP and TCP.
 * 
 * @section LICENSE
 * Copyright (c) 2009, Floris Chabert, Simon Vetter. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AS IS'' AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO,PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR  
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS  SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SOCKET_USER_H
#define _SOCKET_USER_H

#define AF_INET6 10

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

#define MSG_PEEK 1

/*
 * IPv6 address.
 */
struct in6_addr
{
	union
	{
		u8_t u6_addr8[16];
		u16_t u6_addr16[8];
		u32_t u6_addr32[4];
	};
};

/*
 * IPv6 sockaddr.
 */
struct sockaddr_in6
{
	u16_t sin6_family; // AF_INET6
	u16_t sin6_port; // Transport layer port
	u32_t sin6_flowinfo; // IPv6 flow information
	struct in6_addr sin6_addr; // IPv6 address
	u32_t sin6_scope_id; // Scope ID
};

/*
 * Create a socket.
 *
 * @param family must be AF_INET6 (IPv6)
 * @param type is the protocol type used: SOCK_DGRAM (UDP) or SOCK_STREAM (TCP)
 * @param proto. wait... what ?
 * @return the socket identifier.
 */
u32_t socket(u16_t family, u8_t type, u8_t proto);

/*
 * Receive data from the socket.
 * 
 * @param socket is the socket idenfier.
 * @param buff is a pointer to the received data.
 * @param nbytes is the number of byte received.
 * @param flags could contain the following flags: MSG_PEEK
 * @param from is a pointer to an IPv6 sockaddr struct.
 * @param fromaddrlen is a pointer to the size of from.
 * @return the number of bytes read.
 */
u32_t recvfrom(u32_t socket, void *buff, u16_t len, u8_t flags, struct sockaddr_in6* from, u32_t *fromaddrlen);

u32_t sendto(u32_t socket, const void *buff, u16_t len, u8_t flags, const struct sockaddr_in6 *to, u32_t toaddrlen);

#endif
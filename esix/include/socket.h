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
#define MSG_DONTWAIT 2

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

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

/*
 * IPv6 sockaddr.
 */
struct sockaddr_in6
{
	u16_t sin6_family; // AF_INET6
	u16_t sin6_port; // Transport layer port
	u32_t sin6_flowinfo; // IPv6 flow information, not used
	struct in6_addr sin6_addr; // IPv6 address
	u32_t sin6_scope_id; // Scope ID, not used
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
 * Change properties of a socket.
 *
 * @param socket is the socket idenfier.
 * @param address is a pointer to the IPv6 sockaddr stuct to be used.
 * @param addrlen is the size of address.
 * @return the socket identifier.
 */
u32_t bind(u32_t socket, const struct sockaddr_in6 *address, u32_t addrlen);

/*
 * Initiate a connection.
 *
 * @param socket is the socket idenfier.
 * @param to is a pointer to the IPv6 sockaddr stuct to be used.
 * @param addrlen is the size of to.
 * @return 0 in success.
 */
int connect(u32_t socket, const struct sockaddr_in6 *to, u32_t addrlen);

/*
 * Listen for connections.
 *
 * @param socket is the socket idenfier.
 * @param num is the number of connections allowed on the socket.
 * @return 0 in success.
 */
int listen(u32_t socket, u32_t num);

/*
 * Accept a connection.
 *
 * @param socket is the socket idenfier.
 * @param address is a pointer to the IPv6 sockaddr stuct to be used.
 * @param addrlen is the size of address.
 * @return a socket identifier for the connection.
 */
u32_t accept(u32_t socket, struct sockaddr_in6 *address, u32_t *addrlen);

/*
 * Close a socket.
 *
 * @param socket is the socket idenfier.
 * @return 0 in success
 */
int close(u32_t socket);

/*
 * Receive data through a socket (for a connected socket).
 * 
 * @param socket is the socket idenfier.
 * @param buff is a pointer to a buffer where the received data can be copied.
 * @param len is the length (in bytes) os the buffer.
 * @param flags could contain the following flags: MSG_PEEK, MSG_DONTWAIT.
 * @return the number of bytes read.
 */
u32_t recv(u32_t socket, void *buff, u16_t len, u8_t flags);

/*
 * Send data through a socket (for a connected socket).
 *
 * @paramsocket is the socket idenfier.
 * @param buff is a pointer to a buffer containing the data to be sent.
 * @param len is the number of bytes to send.
 * @param flags is not used (for now).
 * @param from is a pointer to an IPv6 sockaddr struct (containing destination details).
 * @param fromaddrlen is a pointer to the size of from.
 * @return the number of bytes sent.
 */
u32_t send(u32_t socket, const void *buff, u16_t len, u8_t flags);

/*
 * Receive data from the socket.
 * 
 * @param socket is the socket idenfier.
 * @param buff is a pointer to a buffer where the received data can be copied.
 * @param len is the length (in bytes) os the buffer.
 * @param flags could contain the following flags: MSG_PEEK, MSG_DONTWAIT.
 * @param from is a pointer to an IPv6 sockaddr struct (where sender details will be copied).
 * @param fromaddrlen is a pointer to the size of from.
 * @return the number of bytes read.
 */
u32_t recvfrom(u32_t socket, void *buff, u16_t len, u8_t flags, struct sockaddr_in6* from, u32_t *fromaddrlen);

/*
 * Send data through a socket.
 *
 * @paramsocket is the socket idenfier.
 * @param buff is a pointer to a buffer containing the data to be sent.
 * @param len is the number of bytes to send.
 * @param flags is not used (for now).
 * @param from is a pointer to an IPv6 sockaddr struct (containing destination details).
 * @param fromaddrlen is a pointer to the size of from.
 * @return the number of bytes sent.
 */
u32_t sendto(u32_t socket, const void *buff, u16_t len, u8_t flags, const struct sockaddr_in6 *to, u32_t toaddrlen);

#endif
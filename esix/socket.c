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

#include "config.h"
#include "tools.h"
#include "socket.h"
#include "include/socket.h"
#include "udp6.h"
#include "tcp6.h"

const struct in6_addr in6addr_any = {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}};
const struct in6_addr in6addr_loopback = {{{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0}}};

u32_t socket(u16_t family, u8_t type, u8_t proto)
{	
	u32_t socket = 3;
	u16_t port = 1024;
	
	if((family != AF_INET6) | ((type != SOCK_DGRAM) && (type != SOCK_STREAM)))
		return -1;
		
	while(esix_socket_get_index(socket) >= 0)
		socket++; // give the first socket identifier available
	
	while(esix_socket_get_port_index(hton16(port), type) >= 0)
		port++; // give the first port available (>= 1024)
			
	if(esix_socket_add(socket, type, hton16(port)) < 0)
		return -1;
	return socket;
}

u32_t bind(u32_t socket, const struct sockaddr_in6 *address, u32_t addrlen)
{
	int i;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return -1;
		
	if(esix_socket_get_port_index(address->sin6_port, sockets[i]->type) < 0)
		sockets[i]->hport = address->sin6_port;
	else
		return -1;
		
	esix_memcpy(&sockets[i]->haddr, &address->sin6_addr, 16);
	
	return socket;
}

int connect(u32_t socket, const struct sockaddr_in6 *to, u32_t addrlen)
{
	int i;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return -1;

	esix_memcpy(&sockets[i]->raddr, &to->sin6_addr, 16);
	sockets[i]->rport = to->sin6_port;

	if(sockets[i]->type == SOCK_STREAM)
	{
		if(sockets[i]->state != CLOSED)
			return -1;
				
		sockets[i]->state = SYN_SENT;	
		esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, sockets[i]->seqn, 0, SYN, NULL, 0);
		
		while(sockets[i]->state != ESTABLISHED);	
	}
	return 0;
}

int listen(u32_t socket, u32_t num)
{
	int i;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return -1;
	
	if(sockets[i]->type != SOCK_STREAM)
		return -1;
		
	sockets[i]->state = LISTEN;
	return 0;
}

u32_t accept(u32_t socket, struct sockaddr_in6 *address, u32_t *addrlen)
{
	u32_t sockc = 3;
	int i, j;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return -1;
	
	if((sockets[i]->type != SOCK_STREAM) || (sockets[i]->state != LISTEN))
		return -1;	
	
	while(sockets[i]->state != ESTABLISHED);
	sockets[i]->state = LISTEN;
	
	// we create a new socket for the session
	while(esix_socket_get_index(sockc) >= 0)
		sockc++;
		
	if((j = esix_socket_add(sockc, SOCK_STREAM, sockets[i]->hport)) < 0)
		return -1;
		
	sockets[j]->session = 1;
	sockets[j]->state = ESTABLISHED;
	sockets[j]->rport = sockets[i]->rport;
	sockets[j]->seqn = sockets[i]->seqn;
	sockets[j]->ackn = sockets[i]->ackn;
	esix_memcpy(&sockets[j]->raddr, &sockets[i]->raddr, 16);
	
	address->sin6_port = sockets[i]->rport;
	esix_memcpy(&address->sin6_addr, &sockets[i]->raddr, 16);
	
	return sockc;
}

int close(u32_t socket)
{
	int i;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return -1;
	
	if(sockets[i]->type == SOCK_DGRAM)
	{
		esix_socket_remove_row(i);
	}
	else if(sockets[i]->state == CLOSED)
	{
		esix_socket_remove_row(i);
	}
	else if(sockets[i]->state == LAST_ACK)
	{
	}
	else if(sockets[i]->state == ESTABLISHED)
	{
		sockets[i]->state = FIN_WAIT_1;
		esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, sockets[i]->seqn++, sockets[i]->ackn, FIN | ACK, NULL, 0);
	}
	return 0;
}

u32_t recv(u32_t socket, void *buff, u16_t len, u8_t flags)
{
	return recvfrom(socket, buff, len, flags, NULL, NULL);
}

u32_t send(u32_t socket, const void *buff, u16_t len, u8_t flags)
{
	return sendto(socket, buff, len, flags, NULL, 0);
}

u32_t recvfrom(u32_t socket, void *buff, u16_t len, u8_t flags, struct sockaddr_in6* from, u32_t *fromaddrlen)
{
	int i;
	u32_t plen;
	struct udp_packet *udp;
	struct tcp_packet *tcp;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return 0;
	
	if(sockets[i]->type == SOCK_DGRAM)
	{
		while((sockets[i]->received == NULL) && !(flags & MSG_DONTWAIT));
	
		udp = sockets[i]->received;
		if(udp == NULL)
			return 0;
		
		plen = udp->len;
		if(plen > len)
			plen = len;
		esix_memcpy(buff, udp->data, plen);

		if(from != NULL)
		{
			from->sin6_port = udp->s_port;
			esix_memcpy(&from->sin6_addr, &udp->s_addr, 16);
		}
		
		if(!(flags & MSG_PEEK))
		{
			esix_w_free(udp->data);
			esix_w_free(sockets[i]->received);
			sockets[i]->received = NULL;
		}
	}
	else
	{
		while((sockets[i]->state == ESTABLISHED) && (sockets[i]->received == NULL) && !(flags & MSG_DONTWAIT));
		
		tcp = sockets[i]->received;
		if(tcp == NULL)
			return 0;

		plen = tcp->len;
		if(plen > len)
			plen = len;
		esix_memcpy(buff, tcp->data, plen);

		if(!(flags & MSG_PEEK))
		{
			if(tcp->len > 0)
				esix_w_free(tcp->data);
			esix_w_free(sockets[i]->received);
			sockets[i]->received = NULL;
		}
	}

	return plen;
}

u32_t sendto(u32_t socket, const void *buff, u16_t len, u8_t flags, const struct sockaddr_in6 *to, u32_t toaddrlen)
{
	int i;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return -1;
		
	if(sockets[i]->type == SOCK_DGRAM)
	{
		if(len > MAX_UDP_LEN)
			return 0;
	
		if(to != NULL)
			esix_udp_send((struct ip6_addr *) &to->sin6_addr, sockets[i]->hport, to->sin6_port, buff, len);
		else
			esix_udp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, buff, len);
	}
	else
	{
		if(sockets[i]->state != ESTABLISHED)
			return 0;
		
		esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, sockets[i]->seqn, sockets[i]->ackn, ACK, buff, len);	
		sockets[i]->seqn += len;
	}
	
	return len;
}

int esix_socket_add_row(struct socket_table_row *row)
{
	int i;
	for(i = 0; i < ESIX_MAX_SOCK; i++)
	{
		if(sockets[i] == NULL)
		{
			sockets[i] = row;
			break;
		}
	}	
	if(i == ESIX_MAX_SOCK)
		return -1;
	return i;
}

void esix_socket_remove_row(int index)
{
	esix_w_free(sockets[index]);
	sockets[index] = NULL;
}

int esix_socket_add(u32_t socket, u8_t type, u16_t port)
{
	struct socket_table_row *row = esix_w_malloc(sizeof(struct socket_table_row));
	
	row->socket = socket;
	row->type = type;
	row->hport = port;
	esix_memcpy(&row->haddr, &in6addr_any, 16);
	row->state = CLOSED;
	row->session = 0;
	row->seqn = 42;
	row->received = NULL;
	
	return esix_socket_add_row(row);
}

int esix_socket_get_index(u32_t socket)
{
	int j;
	for(j = 0; j<ESIX_MAX_SOCK; j++)
	{
		//check if we already stored this address
		if((sockets[j] != NULL) &&
			((sockets[j]->socket == socket)))
		{
			return j;
		}
	}
	return -1;
}

int esix_socket_get_port_index(u16_t port, u8_t type)
{
	int j;
	
	for(j = 0; j<ESIX_MAX_SOCK; j++)
	{
		//check if we already stored this address
		if((sockets[j] != NULL) &&
			(sockets[j]->session == 0) &&
			(sockets[j]->hport == port) &&
			(sockets[j]->type == type))
		{
			return j;
		}
	}
	return -1;
}

int esix_socket_get_session_index(u16_t hport, u16_t rport, struct ip6_addr raddr)
{
	int j;
	
	for(j = 0; j<ESIX_MAX_SOCK; j++)
	{
		//check if we already stored this address
		if((sockets[j] != NULL) &&
			(sockets[j]->session != 0) &&
			(sockets[j]->hport == hport) &&
			(sockets[j]->rport == rport) &&
			!esix_memcmp(&sockets[j]->raddr, &raddr, 16))
		{
			return j;
		}
	}
	return -1;
}

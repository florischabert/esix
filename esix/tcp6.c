/**
 * @file
 * TCP protocol implementation.
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

#include "tcp6.h"
#include "tools.h"
#include "icmp6.h"
#include "intf.h"
#include "include/socket.h"
#include "socket.h"

void esix_tcp_process(struct tcp_hdr *t_hdr, int len, struct ip6_hdr *ip_hdr)
{
	int i;
	struct tcp_packet *packet;

	i = esix_socket_get_session_index(t_hdr->d_port, t_hdr->s_port, ip_hdr->saddr);	
	if(i < 0)
		i = esix_socket_get_port_index(t_hdr->d_port, SOCK_STREAM);	
	
	if((i < 0) || (esix_memcmp(&ip_hdr->daddr, &sockets[i]->haddr, 16) && 
                  esix_memcmp(&sockets[i]->haddr, &in6addr_any, 16)))
	{
		uart_printf("TCP port %x unreachable\n", hton16(t_hdr->d_port));
		esix_tcp_send(&ip_hdr->saddr, t_hdr->d_port, t_hdr->s_port, 0, ntoh32(t_hdr->seqn)+1, RST | ACK, NULL, 0);
	}
	else
	{
		uart_printf("socket: %x\n", sockets[i]->state);
		if(sockets[i]->state == CLOSED)
		{
			esix_tcp_send(&ip_hdr->saddr, t_hdr->d_port, t_hdr->s_port, sockets[i]->seqn, ntoh32(t_hdr->seqn)+1, RST | ACK, NULL, 0);
		}
		if(sockets[i]->state == LISTEN)
		{	
			if(t_hdr->flags & SYN)
			{
				sockets[i]->rport = t_hdr->s_port;
				esix_memcpy(&sockets[i]->raddr, &ip_hdr->saddr, 16);
				sockets[i]->state = SYN_RECEIVED;
				sockets[i]->ackn = hton32(t_hdr->seqn);
				esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, sockets[i]->seqn++, ++sockets[i]->ackn, SYN | ACK, NULL, 0);
			}		
		}
		else if(sockets[i]->state == SYN_RECEIVED)
		{
			if(t_hdr->flags & SYN)
				sockets[i]->state = LISTEN; // FIXME
				
			if(t_hdr->flags & ACK)
			{
				sockets[i]->state = ESTABLISHED;
			}
		}
		else if(sockets[i]->state == SYN_SENT)
		{
			if(t_hdr->flags & ACK)
			{
				sockets[i]->state = ESTABLISHED;
				if(t_hdr->flags & SYN)
				{
					sockets[i]->ackn = hton32(t_hdr->seqn);
					esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, ++sockets[i]->seqn, ++sockets[i]->ackn, ACK, NULL, 0);
				}
			}
		}
		else if(sockets[i]->state == ESTABLISHED)
		{
			if(t_hdr->flags & SYN)
				sockets[i]->state = LISTEN;
				
			if(t_hdr->flags & FIN)
			{
				sockets[i]->state = LAST_ACK;
				esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, sockets[i]->seqn, ++sockets[i]->ackn, FIN | ACK, NULL, 0);
			}
			else if(len - sizeof(struct tcp_hdr) > 0)
			{
				if(sockets[i]->received == NULL)
				{
					packet = esix_w_malloc(sizeof(struct tcp_packet));
					packet->len = len - sizeof(struct tcp_hdr);
					if(packet->len > 0)
					{
						packet->data = esix_w_malloc(packet->len);
						esix_memcpy(packet->data, t_hdr + 1, packet->len);
					}
					packet->next = NULL;
					sockets[i]->received = packet;
					sockets[i]->ackn += packet->len; 
					esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, sockets[i]->seqn, sockets[i]->ackn, ACK, NULL, 0);
				}
				else
					uart_printf("A TCP packet is already in the queue.\n"); // FIXME
			}
		}
		else if(sockets[i]->state == LAST_ACK)
		{
			if(t_hdr->flags & ACK)
			{
				sockets[i]->state = CLOSED;
			}
		}
		else if(sockets[i]->state == FIN_WAIT_1)
		{
			if(t_hdr->flags & ACK)
				sockets[i]->state = FIN_WAIT_2;
		}
		else if(sockets[i]->state == FIN_WAIT_2)
		{
			if((t_hdr->flags & FIN) && (t_hdr->flags & ACK))
			{
				esix_tcp_send(&sockets[i]->raddr, sockets[i]->hport, sockets[i]->rport, ++sockets[i]->seqn, ++sockets[i]->ackn, ACK, NULL, 0);
				sockets[i]->state = CLOSED;
			}
		}
	}
}

void esix_tcp_send(struct ip6_addr *daddr, u16_t s_port, u16_t d_port, u32_t	seqn, u32_t	ackn, u8_t flags, const void *data, u16_t len)
{
	int i;
	struct ip6_addr saddr;
	struct tcp_hdr *hdr = esix_w_malloc(sizeof(struct tcp_hdr) + len);
	hdr->d_port = d_port;
	hdr->s_port = s_port;
	hdr->seqn = hton32(seqn);
	hdr->ackn = hton32(ackn);
	hdr->data_offset = 5 << 4;
	hdr->flags = flags;
	hdr->w_size = hton16(1400);
	hdr->urg_pointer = 0;
	hdr->chksum = 0;
	esix_memcpy(hdr + 1, data, len);

	// get the source address
	if((daddr->addr1 & hton32(0xffff0000)) == hton32(0xfe800000))
		i = esix_intf_get_scope_address(LINK_LOCAL_SCOPE);
	else
		i = esix_intf_get_scope_address(GLOBAL_SCOPE);

	saddr = addrs[i]->addr;
	
	hdr->chksum = esix_ip_upper_checksum(&saddr, daddr, TCP, hdr, len + sizeof(struct tcp_hdr));

	esix_ip_send(&saddr, daddr, 64, TCP, hdr, len + sizeof(struct tcp_hdr));
}


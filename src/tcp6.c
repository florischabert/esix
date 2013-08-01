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
#include "nd6.h"
#include "socket.h"
#include <socket.h>

void esix_tcp_process(const void *payload, int len, const esix_ip6_hdr *ip_hdr)
{
	const struct tcp_hdr *t_hdr = payload;
	int session_sock;

	//do we have enough bytes to proces the header?
	if(len < 20)
	{
		return;
	}
	
	//check the checksum
	if(esix_ip6_upper_checksum(&ip_hdr->src_addr, &ip_hdr->dst_addr, esix_ip6_next_tcp, t_hdr, len) != 0)
		return;

	switch (t_hdr->flags)
	{
		case SYN:
			//try to create a child connection. if if fails, send a RST|ACK right away.
			if((session_sock = esix_socket_create_child(&ip_hdr->src_addr, &ip_hdr->dst_addr, 
				t_hdr->s_port, t_hdr->d_port, SOCK_STREAM)) < 0)
			{
				esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
					ntoh32(t_hdr->ackn)+1, ntoh32(t_hdr->seqn)+1, RST|ACK, NULL, 0);
				return;
			}

			esix_sockets[session_sock].state = SYN_RECEIVED;
			esix_sockets[session_sock].ackn = ntoh32(t_hdr->seqn)+1;
			esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
				esix_sockets[session_sock].seqn, esix_sockets[session_sock].ackn, SYN|ACK, NULL, 0);
			esix_sockets[session_sock].seqn++;

		break;

		case SYN|ACK:
			if((session_sock = esix_find_socket(&ip_hdr->src_addr, &ip_hdr->dst_addr, 
				t_hdr->s_port, t_hdr->d_port, SOCK_STREAM, FIND_CONNECTED)) < 0)
			{
				esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
					ntoh32(t_hdr->ackn)+1, ntoh32(t_hdr->seqn)+1, RST|ACK, NULL, 0);
				return;
			}

			//put the socket in established mode and store remote node's seq number
			if(ntoh32(t_hdr->ackn) == esix_sockets[session_sock].seqn)
			{
				if(esix_sockets[session_sock].state == SYN_SENT)
					esix_sockets[session_sock].state = ESTABLISHED;

				esix_sockets[session_sock].ackn = ntoh32(t_hdr->ackn)+1;
				esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
					esix_sockets[session_sock].seqn, esix_sockets[session_sock].ackn, ACK, NULL, 0);

			}
			
		break;

		case ACK:
		case PSH|ACK:
		case CWR|PSH|ACK:
			if((session_sock = esix_find_socket(&ip_hdr->src_addr, &ip_hdr->dst_addr, 
				t_hdr->s_port, t_hdr->d_port, SOCK_STREAM, FIND_CONNECTED)) < 0)  
			{
				esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
					ntoh32(t_hdr->ackn), ntoh32(t_hdr->seqn), RST|ACK, NULL, 0);
				return;
			}

			//remove every ack'ed packet from our send queue
			esix_socket_expire_e(session_sock, ntoh32(t_hdr->ackn));
			
			//packet sequence OK
			if(ntoh32(t_hdr->seqn) == esix_sockets[session_sock].ackn)
			{
				switch(esix_sockets[session_sock].state)
				{
					case SYN_RECEIVED:
						esix_sockets[session_sock].state = ESTABLISHED;
					break;
					case FIN_WAIT_2:
						esix_socket_free_queue(session_sock);
						esix_sockets[session_sock].state = CLOSED;
					break;
					case ESTABLISHED:
						//grab received data, if any
						if((len-((t_hdr->data_offset>>4)*4)) >0)
						{
							if((esix_queue_data(session_sock, (uint8_t*) t_hdr + ((t_hdr->data_offset>>4)*4) ,
									len-(t_hdr->data_offset>>4)*4, NULL, IN)) <0 )
								return;
							esix_sockets[session_sock].ackn += len-((t_hdr->data_offset>>4)*4);
							esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
								esix_sockets[session_sock].seqn, esix_sockets[session_sock].ackn, ACK, NULL, 0);
						}
					break;
					default :
					break;
				}

			}
			else
			{
				//late, retransmitted packet
				esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
					esix_sockets[session_sock].seqn, esix_sockets[session_sock].ackn, ACK, NULL, 0);
			}

		break;

		case FIN:
		case FIN|ACK:
			if((session_sock = esix_find_socket(&ip_hdr->src_addr, &ip_hdr->dst_addr, 
				t_hdr->s_port, t_hdr->d_port, SOCK_STREAM, FIND_CONNECTED)) < 0)
			{
				esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
					ntoh32(t_hdr->ackn)+1, ntoh32(t_hdr->seqn)+1, RST|ACK, NULL, 0);
				return;
			}

			//remove every ack'ed packet from our send queue
			esix_socket_expire_e(session_sock, ntoh32(t_hdr->ackn));

			//is the packet in order?
			if(ntoh32(t_hdr->ackn) == esix_sockets[session_sock].seqn)
			{
				esix_sockets[session_sock].ackn += 1 ;

				switch(esix_sockets[session_sock].state)
				{
					case SYN_RECEIVED:
					case ESTABLISHED:
						esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
							esix_sockets[session_sock].seqn, esix_sockets[session_sock].ackn, FIN|ACK, NULL, 0);
							esix_sockets[session_sock].seqn += 1 ;

						esix_sockets[session_sock].state = FIN_WAIT_2;
					break;

					case FIN_WAIT_1:
						esix_tcp_send(&ip_hdr->dst_addr, &ip_hdr->src_addr, t_hdr->d_port, t_hdr->s_port,
							esix_sockets[session_sock].seqn, esix_sockets[session_sock].ackn, ACK, NULL, 0);

						esix_socket_free_queue(session_sock);
						esix_sockets[session_sock].state = CLOSED;
					break;
					default :
					break;
				}
			}
			
		break;

		case RST:
		case RST|ACK:
			if((session_sock = esix_find_socket(&ip_hdr->src_addr, &ip_hdr->dst_addr, 
				t_hdr->s_port, t_hdr->d_port, SOCK_STREAM, FIND_CONNECTED)) < 0)
				return;

			if(esix_sockets[session_sock].state != CLOSED)
				esix_sockets[session_sock].state = CLOSED;
			esix_socket_free_queue(session_sock);
		break;


		default :
			//uh oh
			break;
	}
}

void esix_tcp_send(const esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr, const uint16_t s_port, const uint16_t d_port, 
	const uint32_t seqn, const uint32_t ackn, const uint8_t flags, const void *data, const uint16_t len)
{
	struct tcp_hdr *hdr;
	esix_ip6_addr saddr = *src_addr;

	//check source address
	if(esix_nd6_check_source_addr(&saddr, dst_addr) < 0)
		return;	

	if((hdr = esix_malloc(sizeof(struct tcp_hdr) + len)) == NULL)
		return;
	
	hdr->d_port = d_port;
	hdr->s_port = s_port;
	hdr->seqn = hton32(seqn);
	hdr->ackn = hton32(ackn);
	hdr->data_offset = (5 << 4); //we don't support any option yet...
	hdr->flags = flags;
	hdr->w_size = hton16(1400);
	hdr->urg_pointer = 0;
	hdr->chksum = 0;
	esix_memcpy(hdr + 1, data, len);
	
	hdr->chksum = esix_ip6_upper_checksum(&saddr, dst_addr, esix_ip6_next_tcp, hdr, len + sizeof(struct tcp_hdr));

	esix_ip6_send(&saddr, dst_addr, DEFAULT_TTL, esix_ip6_next_tcp, hdr, len + sizeof(struct tcp_hdr));

	esix_free(hdr);
}


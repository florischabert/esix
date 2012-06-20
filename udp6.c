/**
 * @file
 * UDP protocol implementation.
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

#include "tools.h"
#include "udp6.h"
#include "icmp6.h"
#include "intf.h"
#include "include/socket.h"
#include "socket.h"

void esix_udp_process(const struct udp_hdr *u_hdr, int len, const struct ip6_hdr *ip_hdr)
{
	int sock;
	struct sockaddr_in6 sockaddr;

	//do we have enough bytes to proces the header?
	if(len < 8)
	{
		return;
	}
	
	// check the checksum
	if(esix_ip_upper_checksum(&ip_hdr->saddr, &ip_hdr->daddr, UDP, u_hdr, len) != 0)
		return;

	if((sock = esix_find_socket(&ip_hdr->saddr, &ip_hdr->daddr, u_hdr->s_port, u_hdr->d_port, 
		UDP, FIND_LISTEN)) < 0)
	{
		//don't send port unreach in response to multicasts
		if( (ip_hdr->daddr.addr1 & hton32(0xff000000)) != hton32(0xff000000))
			esix_icmp_send_unreachable(ip_hdr, PORT_UNREACHABLE); 	
		return;
	}

	esix_memcpy(&sockaddr.sin6_addr, &ip_hdr->saddr, 16);
	sockaddr.sin6_port = u_hdr->s_port;
	esix_queue_data(sock, u_hdr+1, ntoh16(u_hdr->len)-sizeof(struct udp_hdr), &sockaddr, IN);

	return;
}

void esix_udp_send(const struct ip6_addr *saddr, const struct ip6_addr *daddr, u16_t s_port, u16_t d_port, const void *data, u16_t len)
{
	struct udp_hdr *hdr = esix_w_malloc(sizeof(struct udp_hdr) + len);
	if(hdr == NULL)
		return;

	hdr->d_port = d_port;
	hdr->s_port = s_port;
	hdr->len = hton16(len + sizeof(struct udp_hdr));
	hdr->chksum = 0;
	esix_memcpy(hdr + 1, data, len);

	hdr->chksum = esix_ip_upper_checksum(saddr, daddr, UDP, hdr, len + sizeof(struct udp_hdr));
	
	esix_ip_send(saddr, daddr, DEFAULT_TTL, UDP, hdr, len + sizeof(struct udp_hdr));

	esix_w_free(hdr);
}

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

void esix_udp_process(struct udp_hdr *u_hdr, int len, struct ip6_hdr *ip_hdr)
{
	toggle_led();
	int i;
	struct udp_packet *packet;

	i = esix_socket_get_port_index(u_hdr->d_port, SOCK_DGRAM);	
	if(i < 0)
	{
		// esix_icmp_send_unreachable
		uart_printf("UDP port %x unreachable\n", u_hdr->d_port); // FIXME
		esix_icmp_send_unreachable(ip_hdr, PORT_UNREACHABLE);
	}
	else
	{
		// TODO: linked list for received data
		if(sockets[i]->received == NULL)
		{
			packet = esix_w_malloc(sizeof(struct udp_packet));
			packet->s_port = u_hdr->s_port;
			esix_memcpy(&packet->s_addr, &ip_hdr->saddr, 16);
			packet->len = ntoh16(u_hdr->len) - sizeof(struct udp_hdr);
			packet->data = esix_w_malloc(packet->len);
			esix_memcpy(packet->data, u_hdr + 1, packet->len);
			packet->next = NULL;
			sockets[i]->received = packet;
		}
		else
			uart_printf("An UDP packet is already in the queue.\n"); // FIXME 
	}
	
	return;
}

void esix_udp_send(struct ip6_addr *daddr, u16_t s_port, u16_t d_port, const void *data, u16_t len)
{
	int i;
	struct ip6_addr saddr;
	struct udp_hdr *hdr = esix_w_malloc(sizeof(struct udp_hdr) + len);
	hdr->d_port = d_port;
	hdr->s_port = s_port;
	hdr->len = hton16(len + sizeof(struct udp_hdr));
	hdr->chksum = 0;
	esix_memcpy(hdr + 1, data, len);

	// get the source address
	if((daddr->addr1 & hton32(0xffff0000)) == hton32(0xfe800000))
		i = esix_intf_get_scope_address(LINK_LOCAL_SCOPE);
	else
		i = esix_intf_get_scope_address(GLOBAL_SCOPE);

	saddr = addrs[i]->addr;
	
	hdr->chksum = esix_ip_upper_checksum(&saddr, daddr, UDP, hdr, len + sizeof(struct udp_hdr));
	
	esix_ip_send(&saddr, daddr, 64, UDP, hdr, len + sizeof(struct udp_hdr));
}

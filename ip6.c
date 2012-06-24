/**
 * @file
 * esix ipv6 stack, ip packets processing functions.
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

#include "ip6.h"
#include "udp6.h"
#include "tcp6.h"
#include "intf.h"
#include "tools.h"
#include "icmp6.h"

static esix_ip6_upper_handler upper_handlers[] = {
	{ esix_ip6_next_icmp, esix_icmp6_process },
	{ esix_ip6_next_udp, esix_udp_process },
	{ esix_ip6_next_tcp, esix_tcp_process }
};

 int esix_ip6_addr_match(const esix_ip6_addr addr1, const esix_ip6_addr addr2)
{
	int i;
	int does_match = 1;

	for (i = 0; i < 4; i++) {
		if (addr1.raw[i] != addr2.raw[i]) {
			does_match = 0;
			break;
		}
	}
	return does_match;
}

void esix_ip6_process(const void *payload, int len)
{
	const esix_ip6_hdr *hdr = payload;
	esix_ip6_upper_handler *handler;
	int i, pkt_for_us;

	//check if we have enough data to at least read the header
	//and if we actually have an IPv6 packet
	if((len < 40)  || 
		((hdr->ver_tc_flowlabel&hton32(0xf0000000)) !=  hton32(0x06 << 28)))
		return; 

	//now check if the ethernet frame is long enough to carry the entire ipv6 packet
	if(len < (ntoh16(hdr->payload_len) + 40))
		return;

	//check if the packet belongs to us
	pkt_for_us = 0;

	for(i=0; i<ESIX_MAX_IPADDR;i++)
	{
		//go through every entry of our address table and check word by word
		if( (addrs[i] != NULL) &&
			( hdr->dst_addr.raw[0] == addrs[i]->addr.raw[0] ) &&
			( hdr->dst_addr.raw[1] == addrs[i]->addr.raw[1] ) &&
			( hdr->dst_addr.raw[2] == addrs[i]->addr.raw[2] ) &&
			( hdr->dst_addr.raw[3] == addrs[i]->addr.raw[3] ))
		{
			pkt_for_us = 1;
			break;
		}
	}

	//drop the packet in case it doesn't belong to us
	if(pkt_for_us==0)
		return;
	
	//check the hop limit value (should be > 0)
	if(hdr->hlimit == 0)
	{
		esix_icmp6_send_ttl_expired(hdr);
		return;
	}

	esix_foreach(handler, upper_handlers) {
		if (hdr->next_header == handler->next) {
			handler->process((void *) (hdr + 1), ntoh16(hdr->payload_len), hdr);
			break;
		}
	}

}

void esix_ip6_send(const esix_ip6_addr src_addr, const esix_ip6_addr dst_addr, uint8_t hlimit, esix_ip6_next next, const void *payload, int len)
{
	esix_ip6_hdr *hdr;
	int i, route_index, dest_onlink;
	esix_eth_addr lla;
	
	hdr = malloc(sizeof(esix_ip6_hdr) + len);
	//hmmm... I smell gas...
	if(hdr == NULL)
		return;
	
	hdr->ver_tc_flowlabel = hton32(6 << 28);
	hdr->payload_len = hton16(len);
	hdr->next_header = next;
	hdr->hlimit = hlimit;
	hdr->src_addr = src_addr;
	hdr->dst_addr = dst_addr;
	esix_memcpy(hdr + 1, payload, len);

	route_index = -1;
	//routing
	for(i=0; i < 4; i++)
	{		
		if(	(routes[i] != NULL ) &&
			((dst_addr.raw[0] & routes[i]->mask.raw[0]) == routes[i]->addr.raw[0]) &&
			((dst_addr.raw[1] & routes[i]->mask.raw[1]) == routes[i]->addr.raw[1]) &&
			((dst_addr.raw[2] & routes[i]->mask.raw[2]) == routes[i]->addr.raw[2]) &&
			((dst_addr.raw[3] & routes[i]->mask.raw[3]) == routes[i]->addr.raw[3]))
		{
			route_index = i;
			break;
		}
	}

	//sorry dude, we didn't find any matching route...
	if(route_index < 0)
	{
		free(hdr);
		return;
	}

	// try to find our next hop lla
	if(routes[route_index]->next_hop.raw[0] == 0 && routes[route_index]->next_hop.raw[1] == 0 &&
		routes[route_index]->next_hop.raw[2] == 0 && routes[route_index]->next_hop.raw[3] == 0)
	{
		//onlink route (next hop is destination addr)
		dest_onlink	= 1;

		//if we're sending to a multicast address, don't try to look up a lla,
		//we can compute it
		if(	(dst_addr.raw[0] & hton32(0xff000000)) == hton32(0xff000000))
		{
			lla.raw[0]	=	0x3333;
			lla.raw[1]	=	(uint16_t) dst_addr.raw[3];
			lla.raw[2]	= 	(uint16_t) (dst_addr.raw[3] >> 16);

			esix_eth_send(lla, esix_eth_type_ip6, hdr, len + sizeof(esix_ip6_hdr));

			return;
		}
		else
			//it must be unicast, use the neighbor table.
			i = esix_intf_get_neighbor_index(&dst_addr, INTERFACE);
	}
	else
	{
		//use a router
		dest_onlink	= 0;
		i = esix_intf_get_neighbor_index(&routes[route_index]->next_hop, INTERFACE);
	}

	if(i >= 0) 
	{
		//is it reachable?
		if(neighbors[i]->flags.status == ND_REACHABLE ||
			neighbors[i]->flags.status == ND_STALE)
		{
			//packet leaves here.
			esix_eth_send(neighbors[i]->lla, esix_eth_type_ip6, hdr, len + sizeof(esix_ip6_hdr));
		}
		else
		{
			free(hdr);
			return;
		}
	}
	else
	{
		// we have to send a neighbor solicitation
		if((i=esix_intf_get_type_address(LINK_LOCAL)) >= 0)
		{
			if(dest_onlink)
				esix_icmp6_send_neighbor_sol(&addrs[i]->addr, &dst_addr);
			else
				esix_icmp6_send_neighbor_sol(&addrs[i]->addr, &routes[route_index]->next_hop);
		}
		free(hdr);
	}
	return;
}

uint16_t esix_ip6_upper_checksum(const esix_ip6_addr src_addr, const esix_ip6_addr dst_addr, esix_ip6_next next, const void *payload, int len)
{
	uint32_t sum = 0;
	uint16_t const *data;
	uint16_t *src_addr16 = (uint16_t *)src_addr.raw;
	uint16_t *dst_addr16 = (uint16_t *)dst_addr.raw;

	// IPv6 pseudo-header sum : src_addr, dst_addr, type and payload lenght
	for(data = src_addr16; data < src_addr16+1; data++)
		sum += *data;
	for(data = dst_addr16; data < dst_addr16+1; data++)
		sum += *data;
	sum += hton16(len);
	sum += hton16(next);

	// payload sum
	for(data = payload; len > 1; len -= 2)
		sum += *data++;
	if(len)
		sum += *((uint8_t *) data);

	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	
	return (uint16_t) ~sum;
}

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
#include "intf.h"
#include "tools.h"
#include "icmp6.h"

/**
 * esix_received_frame : processes incoming packets, does sanity checks,
 * then passes the payload to the corresponding upper layer.
 */
void esix_ip_process(void *packet, int len)
{
	struct ip6_hdr *hdr = packet;
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
			( hdr->daddr.addr1 == addrs[i]->addr.addr1 ) &&
			( hdr->daddr.addr2 == addrs[i]->addr.addr2 ) &&
			( hdr->daddr.addr3 == addrs[i]->addr.addr3 ) &&
			( hdr->daddr.addr4 == addrs[i]->addr.addr4 ))
		{
			pkt_for_us = 1;
			break;
		}
	}

	//drop the packet in case it doesn't belong to us
	if(pkt_for_us==0)
	{
		uart_printf("packet received but not for us\n");
		return;
	}
	
	//check the hop limit value (should be > 0)
	if(hdr->hlimit == 0)
	{
		esix_icmp_send_ttl_expired(hdr);
		return;
	}

	//determine what to do next
	switch(hdr->next_header)
	{
		case ICMP:		
			esix_icmp_process((struct icmp6_hdr *) (hdr + 1), 
				ntoh16(hdr->payload_len), hdr);
			break;

		case UDP:
			esix_udp_process((struct udp_hdr *) (hdr + 1),
				ntoh16(hdr->payload_len), hdr);	
			break;
		
		case TCP:
			esix_tcp_process((struct tcp_hdr *) (hdr + 1),
				ntoh16(hdr->payload_len), hdr);
			break;
			
		//unknown (unimplemented) IP type
		default:
			uart_printf("unknown packet received, next_header: %x\n", hdr->next_header);
	}
}

/*
 * Compute upper-level checksum
 */
u16_t esix_ip_upper_checksum(struct ip6_addr *saddr, struct ip6_addr *daddr, u8_t proto, void *payload, u16_t len)
{
	u32_t sum = 0;
	u16_t *data;
	
	// IPv6 pseudo-header sum : saddr, daddr, type and payload lenght
	for(data = (u16_t *) saddr; data < (u16_t *) (saddr+1); data++)
		sum += *data;
	for(data = (u16_t *) daddr; data < (u16_t *) (daddr+1); data++)
		sum += *data;
	sum += hton16(len);
	sum += hton16(proto);

	// payload sum
	for(data = payload; len > 1; len -= 2)
		sum += *data++;
	if(len)
		sum += *((u8_t *) data);

	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	
	return (u16_t) ~sum;
}

/*
 * Send an IPv6 packet.
 */
void esix_ip_send(struct ip6_addr *saddr, struct ip6_addr *daddr, u8_t hlimit, u8_t type, void *data, u16_t len)
{
	struct ip6_hdr *hdr = esix_w_malloc(sizeof(struct ip6_hdr) + len);
	int i, route_index, dest_onlink;
	esix_ll_addr lla;
	
	//hmmm... I smell gas...
	if(hdr == NULL)
		return;
	
	hdr->ver_tc_flowlabel = hton32(6 << 28);
	hdr->payload_len = hton16(len);
	hdr->next_header = type;
	hdr->hlimit = hlimit;
	hdr->saddr = *saddr;
	hdr->daddr = *daddr;
	esix_memcpy(hdr + 1, data, len);
	esix_w_free(data);

	route_index = -1;
	//routing
	for(i=0; i < 4; i++)
	{
		/*
		if(routes[i] != NULL)
			uart_printf("\n\ndaddr : %x %x %x %x\nroute : %x %x %x %x\n", 
				daddr->addr1, daddr->addr2 , daddr->addr3 , daddr->addr4,
				routes[i]->mask.addr1, routes[i]->mask.addr2 , routes[i]->mask.addr3 , routes[i]->mask.addr4);
		*/
		
		if(	(routes[i] != NULL ) &&
			((daddr->addr1 & routes[i]->mask.addr1) == routes[i]->addr.addr1) &&
			((daddr->addr2 & routes[i]->mask.addr2) == routes[i]->addr.addr2) &&
			((daddr->addr3 & routes[i]->mask.addr3) == routes[i]->addr.addr3) &&
			((daddr->addr4 & routes[i]->mask.addr4) == routes[i]->addr.addr4))
		{
			route_index = i;
			break;
		}
	}

	//sorry dude, we didn't find any matching route...
	if(route_index < 0)
	{
		uart_printf("esix_ip_send : no matching route found\n");
		esix_w_free(hdr);
		return;
	}
	// try to find our next hop lla
	if(routes[route_index]->next_hop.addr1 == 0 && routes[route_index]->next_hop.addr2 == 0 &&
		routes[route_index]->next_hop.addr3 == 0 && routes[route_index]->next_hop.addr4 == 0)
	{
		//onlink route (next hop is destination addr)
		dest_onlink	= 1;

		//if we're sending to a multicast address, don't try to look up a lla,
		//we can compute it
		if(	(daddr->addr1 & hton32(0xff000000)) == hton32(0xff000000))
		{
			lla[0]	=	0x3333;
			lla[1]	=	(u16_t) daddr->addr4;
			lla[2]	= 	(u16_t) (daddr->addr4 >> 16);

			esix_w_send_packet(lla, hdr, len + sizeof(struct ip6_hdr));
			return;
		}
		else
			//it must be unicast, use the neighbor table.
			i = esix_intf_get_neighbor_index(daddr, INTERFACE);
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
			esix_w_send_packet(neighbors[i]->lla, hdr, len + sizeof(struct ip6_hdr));
		}
		else
		{
			uart_printf("esix_ip_send : neighbor is unreachable\n");
			esix_w_free(hdr);
			return;
		}
	}
	else
	{
		// we have to send a neighbor solicitation
		uart_printf("packet ready to be sent, but don't now the lla\n");
		if((i=esix_intf_get_scope_address(LINK_LOCAL_SCOPE)) >= 0)
		{
			if(dest_onlink)
				esix_icmp_send_neighbor_sol(&addrs[i]->addr, daddr);
			else
				esix_icmp_send_neighbor_sol(&addrs[i]->addr, &routes[route_index]->next_hop);
		}
		esix_w_free(hdr);
	}
	return;
}

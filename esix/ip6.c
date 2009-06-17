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

/**
 * esix_received_frame : processes incoming packets, does sanity checks,
 * then passes the payload to the corresponding upper layer.
 */
void esix_received_frame(struct ip6_hdr *hdr, int length)
{
	int i, pkt_for_us;
	//check if we have enough data to at least read the header
	//and if we actually have an IPv6 packet
	if((length < 40)  || 
		((hdr->ver_tc_flowlabel&hton32(0xf0000000)) !=  hton32(0x06 << 28)))
		return; 

	//now check if the ethernet frame is long enough to carry the entire ipv6 packet
	if(length < (ntoh16(hdr->payload_len) + 40))
		return;
	
	//check if the packet belongs to us
	pkt_for_us = 0;

	for(i=0; i<ESIX_MAX_IPADDR;i++)
	{
		//go through every entry of our address table and check word by word
		if( (addrs[i] != NULL) &&
			( hdr->daddr1 == (addrs[i]->addr.addr1) ) &&
			( hdr->daddr2 == (addrs[i]->addr.addr2) ) &&
			( hdr->daddr3 == (addrs[i]->addr.addr3) ) &&
			( hdr->daddr4 == (addrs[i]->addr.addr4) ))
		{
			pkt_for_us = 1;
			break;
		}

	}

	//drop the packet in case it doesn't
	if(pkt_for_us==0)
		return;

	//check the hop limit value (should be > 0)
	if(hdr->hlimit == 0)
	{
		esix_send_ttl_expired(hdr);
		return;
	}

	//determine what to do next
	switch(hdr->next_header)
	{
		case ICMP:
			esix_received_icmp((struct icmp6_hdr *) &hdr->data, 
				ntoh16(hdr->payload_len), hdr);
			break;

		//unknown (unimplemented) IP type
		default:
			return;
			break;
	}
}


/**
 * esix_add_to_active_addresses : adds the given IP address to the table. Returns 1 on success.
 */
int esix_add_to_active_addresses(struct esix_ipaddr_table_row *row)
{
	int i=0;

	//look for an empty place where to store our entry.
	while(i<ESIX_MAX_IPADDR)
	{
		if(addrs[i] == NULL)
		{
			addrs[i] = row;
			return 1;
		}
		i++;
	}
	//sorry dude, table was full.
	return 0;
}

/**
 * esix_add_to_active_routes : adds the given route to the routing table. Returns 1 on success.
 */
int esix_add_to_active_routes(struct esix_route_table_row *row)
{
	int i=0;

	while(i<ESIX_MAX_RT)
	{
		if(routes[i]	== NULL)
		{
			routes[i] = row;
			return 1;
		}
		i++;
	}	
	//sorry dude, table was full.
	return 0;
}

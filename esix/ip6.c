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

	//drop the packet in case it doesn't
	if(pkt_for_us==0)
	{
		uart_printf("packed received but not for us\n");
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
			esix_udp_parse((struct udp_hdr *) (hdr + 1),
				ntoh16(hdr->payload_len), hdr);	
		
		case TCP:
			esix_tcp_parse((struct tcp_hdr *) (hdr + 1),
				ntoh16(hdr->payload_len), hdr);
				
		//unknown (unimplemented) IP type
		default:
			uart_printf("unknown packet received, type: %x\n", hdr->next_header);
	}
}

/*
 * Send an IPv6 packet.
 */
void esix_ip_send(struct ip6_addr *saddr, struct ip6_addr *daddr, u8_t hlimit, u8_t type, void *data, u16_t len)
{
	struct ip6_hdr *hdr = esix_w_malloc(sizeof(struct ip6_hdr) + len);
	int i;
	
	hdr->ver_tc_flowlabel = hton32(6 << 28);
	hdr->payload_len = hton16(len);
	hdr->next_header = type;
	hdr->hlimit = hlimit;
	hdr->saddr = *saddr;
	hdr->daddr = *daddr;
	esix_memcpy(hdr + 1, data, len);
	esix_w_free(data);
	
	// do we know the destination lla ?
	i = esix_intf_get_neighbor_index(daddr, INTERFACE);
	if(i >= 0)
		esix_w_send_packet(neighbors[i]->lla, hdr, len + sizeof(struct ip6_hdr));
	else
		// we have to send a neighbor solicitation
		uart_printf("packet ready to be sent, but don't now the lla\n");
}

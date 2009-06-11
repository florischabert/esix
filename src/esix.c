/**
 * @file
 * esix ipv6 stack main file.
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

#include "mmap.h"
#include "uart.h"
#include "../platform/types.h"
#include "include/esix/esix.h"
#include "../platform/ethernet.h"

//general purpose buffer, mainly used to build packets/
static int esix_buffer[ESIX_BUFFER_SIZE];

//this table contains every ip address assigned to the system.
static struct esix_ipaddr_table_row *addrs[ESIX_MAX_IPADDR];

/**
 * esix_init : sets up the esix stack.
 */
void esix_init(void)
{
	int i;
	for(i=0; i<ESIX_MAX_IPADDR; i++)
		addrs[i] = NULL;

	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.
	u16_t mac_addr[3];

	//change here to get a dest mac address out of your ethernet
	//driver if needed. Implementing ether_get_mac_addr() could be simpler.

	ether_get_mac_addr(mac_addr);

	//unicast link local
	struct esix_ipaddr_table_row *ucast_ll = pvPortMalloc(sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	ucast_ll->addr.addr1	= 0xfe800000; // fe80/8 is the link local unicast prefix 
	ucast_ll->addr.addr2	= 0x00000000;

	//last 64 bits
	u8_t	*tmp	= (u8_t*) &mac_addr[1] ; //split a half word in two bytes so we can use them
						//separately
	ucast_ll->addr.addr3	= mac_addr[0] << 16 | tmp[0] << 8 | 0xff;
	ucast_ll->addr.addr4	= 0xfe << 24 | tmp[1] << 16 | mac_addr[2];

	//this one never expires
	ucast_ll->expiration_date	= 0x0;
	ucast_ll->scope			= LINK_LOCAL_SCOPE;

	//TODO perform DAD
	

	//multicast link local associated address (for neighbor discovery)
	struct esix_ipaddr_table_row *mcast_ll = pvPortMalloc(sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	mcast_ll->addr.addr1	= 0xff020000;
	mcast_ll->addr.addr2	= 0x00000000;

	//last 64 bits
	mcast_ll->addr.addr3	= 0x00000001;
	mcast_ll->addr.addr4	= 0xff << 24 | tmp[1] << 16 | mac_addr[2];

	//this one never expires
	mcast_ll->expiration_date	= 0x0;
	mcast_ll->scope			= LINK_LOCAL_SCOPE;

	
	//add them to the table of active addresses
	esix_add_to_active_addresses(ucast_ll);
	esix_add_to_active_addresses(mcast_ll);
}

/**
 * esix_received_frame : processes incoming packets, does sanity checks,
 * then passes the payload to the corresponding upper layer.
 */
void esix_received_frame(struct ip6_hdr *hdr, int length)
{
	int i, pkt_for_us;
	//check if we have enough data to at least read the header
	//and if we actually have an IPv6 packet
	if((length < 40) )//|| ((hdr->ver_tc_flowlabel&0xf0000000) !=  0x06 << 28))
		return; 
	
	//check if the packet belongs to us
	pkt_for_us = 1;
	for(i=0; i<ESIX_MAX_IPADDR;i++)
	{
		//go through every entry of our address table and check word by word
		if( (addrs[i] != NULL) &&
			( hdr->daddr1 == (addrs[i]->addr.addr1) ) &&
			( hdr->daddr2 == (addrs[i]->addr.addr2) ) &&
			( hdr->daddr3 == (addrs[i]->addr.addr3) ) &&
			( hdr->daddr4 == (addrs[i]->addr.addr4) ))
		{
			pkt_for_us = 0;
			break;
		}

	}

	//drop the packet in case it doesn't
	if(pkt_for_us)
		return;

	//check the hop limit value (should be > 0)

	//determine what to do next
	switch(hdr->next_header)
	{
		case ICMP:
			esix_received_icmp((struct icmp6_hdr *) &hdr->data, length-40);
			break;

		//unknown (unimplemented) IP type
		default:
			return;
			break;
	}

	//GPIOF->DATA[1] ^= 1;
}

/**
 * esix_received_icmp : handles icmp packets.
 */
void esix_received_icmp(struct icmp6_hdr *hdr, int length)
{
	//check if we have enough bytes to read the ICMP header
	if(length < 4)
		return;

	//determine what to do next
	switch(hdr->type)
	{
		case NBR_SOL:
			GPIOF->DATA[1] ^= 1;
		default:	
			return;
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
	}
	//sorry dude, table was full.
	return 0;
}

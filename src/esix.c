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

static int esix_buffer[ESIX_BUFFER_SIZE];
//static u128_t addrs[ESIX_MAX_IPADDR];

/**
 * esix_init : sets up the esix stack.
 */
void esix_init(void)
{
	

}

/**
 * esix_received_frame : processes incoming packets, does sanity checks,
 * then passes the payload to the corresponding upper layer.
 */
void esix_received_frame(struct ip6_hdr *hdr, int length)
{
	//check if we have enough data to at least read the header
	//and if we actually have an IPv6 packet
	if((length < 40) )//|| ((hdr->ver_tc_flowlabel&0xf0000000) != 0x60000 ))
		return; 
	
	//check if the packet belongs to us
	//TODO

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

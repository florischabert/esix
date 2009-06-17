/**
 * @file
 * esix stack, ipv6 packets processing functions.
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

#ifndef _IP6_H
#define _IP6_H
	#include "config.h"
	#include "include/esix.h"
	
	//list of IP protocol numbers (some are IPv6-specific)
	#define HOPOPT 	0x00
	#define ICMP	0x3A
	#define NONXT	0x3B
	#define TCP	0x06
	#define UDP	0x11
	#define FRAG	0x2C
	
	#define LINK_LOCAL_SCOPE	0 
	#define GLOBAL_SCOPE	 	1
	#define MCAST_SCOPE		2 //should never be used to send a packet

	#define DEFAULT_TTL		64 	//default TTL when unspecified by
						//router advertisements
	#define DEFAULT_MTU		1500
	
	/**
	 * IPv6 address
	 */
	struct ip6_addr {
		u32_t	addr1;
		u32_t	addr2;
		u32_t	addr3;
		u32_t	addr4;
	};

	/**
	 * IP address table entry
	 */
	struct esix_ipaddr_table_row {
		struct 	ip6_addr addr;	//Actual ip address
		u8_t 	mask;		//netmask (in bits, counting the number of ones
					//starting from the left.
		u32_t	expiration_date;//date at which this entry expires.
					//0 : never expires (for now)
		//u32_t	preferred_exp_date;//date at which this address shouldn't be used if possible
		u8_t	scope;		//LINK_LOCAL_SCOPE or GLOBAL_SCOPE
	
	};

	/**
	 * Route table entry
	 */
	struct esix_route_table_row {
		struct 	ip6_addr addr;	//Network address
		u8_t	mask;
		struct 	ip6_addr next_hop;	//next hop address (should be link-local)
		u32_t	expiration_date;
		u8_t	ttl;		//TTL for this route (per-router TTL values are learnt
					//from router advertisements)
		u16_t	mtu;		//MTU for this route
		u8_t 	interface;	//interface index
	};

	/**
	 * IPv6 header 
	 */
	struct ip6_hdr {
		u32_t 	ver_tc_flowlabel; //version (4bits), trafic class (8 bits), flow label (20 bytes)
		u16_t 	payload_len;	//payload length (next headers + upper protocols)
		u8_t  	next_header;	//next header type
		u8_t  	hlimit; 	//hop limit
		u32_t	saddr1;		//first word of source address
		u32_t	saddr2;		
		u32_t	saddr3;	
		u32_t	saddr4;
		u32_t	daddr1;		//first word of destination address
		u32_t	daddr2;		
		u32_t	daddr3;	
		u32_t	daddr4;
		u32_t	data;
	};
	
#endif

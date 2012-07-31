/**
 * @file
 * UDP protocol implementation
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

#ifndef _UDP6_H
#define _UDP6_H

#include "ip6.h"

	#define MAX_UDP_LEN 1452
	
	struct udp_hdr 
	{
		uint16_t s_port;
		uint16_t	d_port;
		uint16_t	len;
		uint16_t	chksum;
		// UDP data
	} __attribute__((__packed__));

	struct udp_packet
	{
		uint16_t s_port;
		esix_ip6_addr s_addr;
		uint16_t	len;
		uint8_t *data;
		struct udp_packet *next;
	} __attribute__((__packed__));
	
	void esix_udp_process(const void *payload, int len, const esix_ip6_hdr *ip_hdr);
	void esix_udp_send(const esix_ip6_addr *saddr, const esix_ip6_addr *daddr, const uint16_t s_port, 
		const uint16_t d_port, const void *data, const uint16_t len);

#endif

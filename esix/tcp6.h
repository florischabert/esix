/**
 * @file
 * TCP protocol implementation
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

#ifndef _TCP6_H
#define _TCP6_H
	#include "ip6.h"
	
	#define CWR (1 << 7)
	#define ECE (1 << 6)
	#define URG (1 << 5)
	#define ACK (1 << 4)
	#define PSH (1 << 3)
	#define RST (1 << 2)
	#define SYN (1 << 1)
	#define FIN (1 << 0)

	struct tcp_hdr
	{
		u16_t s_port;	
		u16_t d_port;	
		u32_t	seqn;
		u32_t	ackn;
		u8_t data_offset; //mind it's actually 4 data offset
				//bytes plus 4 reserved (all zeroes) bytes here
		u8_t flags;
		u16_t w_size;
		u16_t chksum;
		u16_t urg_pointer;
	} __attribute__((__packed__));
	
	struct tcp_packet
	{
		u16_t	len;
		u8_t *data;
		struct udp_packet *next;
	} __attribute__((__packed__));

	void esix_tcp_process(const struct tcp_hdr *t_hdr, const int len, const struct ip6_hdr *ip_hdr);
	void esix_tcp_send(const struct ip6_addr *saddr, const struct ip6_addr *daddr, const u16_t s_port, 
		const u16_t d_port, const u32_t	seqn, const u32_t ackn, const u8_t flags, const void *data, const u16_t len);
		
#endif

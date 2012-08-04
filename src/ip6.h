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

#include "tools.h"
#include "config.h"
#include <esix.h>

//ip address type
typedef enum
{
	esix_ip6_addr_type_link_local,
	esix_ip6_addr_type_global,
	esix_ip6_addr_type_anycast,
	esix_ip6_addr_type_multicast,
	esix_ip6_addr_type_any
} esix_ip6_addr_type;

/**
 * IPv6 address
 */
 typedef struct {
	uint32_t raw[4];
} __attribute__((__packed__)) esix_ip6_addr;

esix_ip6_addr esix_ip6_addr_create(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3);

/**
 * Compare two ipv6 addresses.
 *
 * @param  addr1 An IPv6 address.
 * @param  addr2 The IPv6 address to compare to.
 * @return Return 1 if the addresses match, or 0.
 */
int esix_ip6_addr_match(const esix_ip6_addr *addr1, const esix_ip6_addr *addr2);

/**
 * Compare a masked IPv6 address with another address.
 * Check whether addr1 & mask == addr2.
 *
 * @param  addr1 The IPv6 address to be masked.
 * @param  mask  The IPv6 address mask.
 * @param  addr2 The IPv6 address to compare to.
 * @return Return 1 if the addresses match, or 0.
 */
int esix_ip6_addr_match_with_mask(const esix_ip6_addr *addr1, const esix_ip6_addr *mask, const esix_ip6_addr *addr2);

/**
 * IPv6 header 
 */
typedef struct {
	uint32_t ver_tc_flowlabel; //version (4bits), trafic class (8 bits), flow label (20 bytes)		
	uint16_t payload_len;
	uint8_t next_header;
	uint8_t hlimit;
	esix_ip6_addr src_addr;
	esix_ip6_addr dst_addr;
} __attribute__((__packed__)) esix_ip6_hdr;

/**
 * Next header type
 */
typedef enum {
	esix_ip6_next_tcp = 0x06,
	esix_ip6_next_udp = 0x11,
	esix_ip6_next_icmp = 0x3a,
} esix_ip6_next;

/**
 * Upper layer handler
 */
typedef struct {
	esix_ip6_next next;
	void (*process)(const void *payload, int len, const esix_ip6_hdr *ip_hdr );
} esix_ip6_upper_handler;

/**
 * Process an incoming ipv6 packet.
 * Do sanity checks and pass its payload to the corresponding upper layer.
 *
 * @param payload The received packet.
 * @param len     Length in bytes of the packet.
 */
void esix_ip6_process(const void *payload, int len);

/**
 * Send an ethernet frame.
 * Encapsule the payload in an ethernet frame and send it.
 *
 * Note: we're not freeing the buffer carrying the payload here, it's up to the upper layer to decide
 * when to do it. Non-retransmitting procotols like UDP or ICMP will typically do it ASAP, but TCP might
 * want to keep it while waiting for an ACK.
 *
 * @param src_addr  Source ipv6 address.
 * @param dst_addr  Destination lipv6 address.
 * @param hlimit    Hop limit.
 * @param next      Next header type.
 * @param payload   Pointer to the payload (the upper-layer packet).
 * @param len       Length in bytes of the payload.
 */
void esix_ip6_send(const esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr, const uint8_t hlimit, const esix_ip6_next next, const void *payload, int len);

/**
 * Compute the upper-level checksum
 *
 * @param src_addr  Source ipv6 address.
 * @param dst_addr  Destination lipv6 address.
 * @param next      Next header type.
 * @param payload   Pointer to the payload (the upper-layer packet).
 * @param len       Length in bytes of the payload.
 * @return          16-bits checksum.
 */
uint16_t esix_ip6_upper_checksum(const esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr, const esix_ip6_next next, const void *payload, int len);


esix_err esix_ip6_work(void);

#endif

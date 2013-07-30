/**
 * @file
 * ethernet frame processing
 *
 * @section LICENSE
 * Copyright (c) 2012, Floris Chabert. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
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

#ifndef _ETH_H
#define _ETH_H

#include "config.h"
#include "tools.h"
#include <esix.h>

/**
 * Ethernet address
 */
typedef struct {
	uint16_t raw[3];
} __attribute__((__packed__)) esix_eth_addr;

/**
 * Compare two ethernet addresses.
 *
 * @param  addr1 An ethernet address.
 * @param  addr2 The ethernet address to compare to.
 * @return Return 1 if the addresses match, or 0.
 */
int esix_eth_addr_match(const esix_eth_addr *addr1, const esix_eth_addr *addr2);

/**
 * Ethernet header 
 */
typedef struct {
	esix_eth_addr dst_addr;
	esix_eth_addr src_addr;
	uint16_t type;
} __attribute__((__packed__)) esix_eth_hdr;

/**
 * Ethertype values
 */
typedef enum {
	esix_eth_type_ip6 = 0x86dd,
} esix_eth_type;

/**
 * Upper layer handler
 */
typedef struct {
	esix_eth_type type;
	void (*process)(const void *payload, int len);
} esix_eth_upper_handler;

esix_eth_addr eth_addr_ntoh(const esix_eth_addr *addr);

/**
 * Process an incoming ethernet frame.
 * Do sanity checks and pass its payload to the corresponding upper layer.
 *
 * @param payload The received frame.
 * @param len     Length in bytes of the frame (including header).
 */
void esix_eth_process(const void *payload, int len);

/**
 * Send an ethernet frame.
 * Encapsule the payload in an ethernet frame and send it.
 *
 * @param dst_addr Destination link-layer address.
 * @param type     Protocol type of the payload.
 * @param payload  Pointer to the payload (the upper-layer packet).
 * @param len      Length in bytes of the payload.
 */
void esix_eth_send(const esix_eth_addr *dst_addr, const esix_eth_type type, const void *payload, int len);

#endif

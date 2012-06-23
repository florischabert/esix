/**
 * @file
 * esix ipv6 stack, ip packets processing functions.
 *
 * @section LICENSE
 * Copyright (c) 2012, Floris Chabert. All rights reserved.
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

#include "eth.h"
#include "intf.h"
#include "tools.h"

extern esix_eth_addr esix_intf_lla;

static int addr_is_multicast(esix_eth_addr addr)
{
	return *(uint16_t *)addr == 0x3333;
}

int esix_eth_addr_match(esix_eth_addr addr1, esix_eth_addr addr2)
{
	int i;
	int does_match = 1;
	for (i = 0; i < 3; i++) {
		if (*(uint16_t *)addr1 != *(uint16_t *)addr2) {
			does_match = 0;
			break;
		}
	}
	return does_match;
}

void esix_eth_addr_cpy(esix_eth_addr dst, const esix_eth_addr src)
{
	int i;
	for (i = 0; i < 3; i++) {
		*(uint16_t *)dst = *(uint16_t *)src;
	}
}

void esix_eth_process(void *bytes, int len)
{
	esix_eth_hdr *hdr = bytes;
	int for_us = 1;
	int i;

	if (esix_eth_addr_match(hdr->dst_addr, esix_intf_lla) ||
		addr_is_multicast(hdr->dst_addr)) {

		if (hdr->type == esix_eth_type_ipv6) {
			esix_ip_process(hdr + 1, len - sizeof(esix_eth_hdr));
		}
	}
}

void esix_send_callback(void *data, int len);

void esix_eth_send(const esix_eth_addr dst_addr, const esix_eth_type type, const void *payload, const uint16_t len)
{
	int i;
	esix_eth_hdr *hdr;

	hdr = malloc(sizeof(esix_eth_hdr) + len);
	if (!hdr) {
		return;
	}

	esix_eth_addr_cpy(hdr->dst_addr, dst_addr);
	esix_eth_addr_cpy(hdr->src_addr, esix_intf_lla);
	
	hdr->type = type;

	memcpy(hdr + 1, payload, len);

	esix_send_callback(hdr, len + sizeof(struct ip6_hdr));
}

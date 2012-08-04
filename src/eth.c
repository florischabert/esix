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

#include "esix.h"
#include "eth.h"
#include "intf.h"
#include "tools.h"

static esix_eth_upper_handler eth_upper_handlers[] = {
	{ esix_eth_type_ip6, esix_ip6_process }
};

static int eth_addr_is_multicast(const esix_eth_addr *addr)
{
	return addr->raw[0] == 0x3333;
}

int esix_eth_addr_match(const esix_eth_addr *addr1, const esix_eth_addr *addr2)
{
	int i;
	int does_match = 1;

	for (i = 0; i < 3; i++) {
		if (addr1->raw[i] != addr2->raw[i]) {
			does_match = 0;
			break;
		}
	}
	return does_match;
}

static esix_eth_addr eth_addr_ntoh(const esix_eth_addr *addr)
{
	esix_eth_addr haddr;
	int i;

	for (i = 0; i < 3; i++) {
		haddr.raw[i] = ntoh16(addr->raw[i]);
	}
	return haddr;
}

static void eth_forward_payload(const esix_eth_type type, const void *payload, int len)
{
	esix_eth_upper_handler *handler;

	esix_foreach(handler, eth_upper_handlers) {
		if (type == handler->type) {
			handler->process(payload, len);
			break;
		}
	}
}

void esix_eth_process(const void *payload, int len)
{
	const esix_eth_hdr *hdr = payload;
	esix_eth_addr dst_addr;
	esix_eth_addr lla = esix_intf_lla();

	dst_addr = eth_addr_ntoh(&hdr->dst_addr);

	if (esix_eth_addr_match(&dst_addr, &lla) ||
		eth_addr_is_multicast(&dst_addr)) {

		eth_forward_payload(ntoh16(hdr->type), hdr + 1, len - sizeof(esix_eth_hdr));
	}
}

void esix_eth_send(const esix_eth_addr *dst_addr, const esix_eth_type type, const void *payload, int len)
{
	esix_buffer *buffer;
	esix_eth_hdr *hdr;
	esix_eth_addr lla = esix_intf_lla();

	hdr = malloc(sizeof(esix_eth_hdr) + len);
	if (!hdr) {
		return;
	}

	hdr->dst_addr = eth_addr_ntoh(dst_addr);
	hdr->src_addr = eth_addr_ntoh(&lla);
	
	hdr->type = hton16(type);

	buffer = malloc(sizeof(*buffer));
	if (!buffer) {
		return;
	}

	memcpy(hdr + 1, payload, len);

	buffer->data = hdr;
	buffer->len = len + sizeof(esix_eth_hdr);

	esix_outqueue_push(buffer);
}

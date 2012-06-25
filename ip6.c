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
#include "udp6.h"
#include "tcp6.h"
#include "intf.h"
#include "tools.h"
#include "icmp6.h"

static esix_ip6_upper_handler ip6_upper_handlers[] = {
	{ esix_ip6_next_icmp, esix_icmp6_process },
	{ esix_ip6_next_udp, esix_udp_process },
	{ esix_ip6_next_tcp, esix_tcp_process }
};

static esix_ip6_addr ip6_addr_null = {{ 0, 0, 0, 0 }};
static esix_ip6_addr ip6_addr_unmasked = {{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }};

static int ip6_addr_is_multicast(const esix_ip6_addr *addr)
{
	return addr->raw[0] == 0xff000000;
}

static esix_ip6_addr ip6_addr_ntoh(const esix_ip6_addr *addr)
{
	esix_ip6_addr haddr;
	int i;

	for (i = 0; i < 4; i++) {
		haddr.raw[i] = ntoh16(addr->raw[i]);
	}
	return haddr;
}

int esix_ip6_addr_match_with_mask(const esix_ip6_addr *addr1, const esix_ip6_addr *mask, const esix_ip6_addr *addr2)
{
	int i;
	int does_match = 1;

	for (i = 0; i < 4; i++) {
		if ((addr1->raw[i] & mask->raw[i]) != addr2->raw[i]) {
			does_match = 0;
			break;
		}
	}
	return does_match;
}

int esix_ip6_addr_match(const esix_ip6_addr *addr1, const esix_ip6_addr *addr2)
{
	return esix_ip6_addr_match_with_mask(addr1, &ip6_addr_unmasked, addr2);
}

static esix_eth_addr ip6_addr_to_eth_multicast(const esix_ip6_addr *addr)
{
	esix_eth_addr lla = {{
		0x3333,
		(uint16_t) addr->raw[3],
		(uint16_t) (addr->raw[3] >> 16)
	}};
	return lla;
}

static int ip6_hdr_get_version(uint32_t ver_tc_flowlabel)
{
	return ver_tc_flowlabel >> 28;
}

static uint32_t ip6_hdr_set_version(int version)
{
	return version << 28;
}

static void ip6_forward_payload(const esix_ip6_next next, const void *payload, int len, const esix_ip6_hdr *hdr)
{
	esix_ip6_upper_handler *handler;

	esix_foreach(handler, ip6_upper_handlers) {
		if (next == handler->next) {
			handler->process(payload, len, hdr);
			break;
		}
	}
}

void esix_ip6_process(const void *payload, int len)
{
	const esix_ip6_hdr *hdr = payload;
	struct esix_ipaddr_table_row **addr_row;

	if (len < sizeof(esix_ip6_hdr)) {
		return;
	}

	if (ip6_hdr_get_version(ntoh32(hdr->ver_tc_flowlabel)) != 6) {
		return; 
	}

	if (hdr->hlimit == 0) {
		esix_icmp6_send_ttl_expired(hdr);
		return;
	}

	if (len != ntoh16(hdr->payload_len) + sizeof(esix_ip6_hdr)) {
		return;
	}

	esix_foreach(addr_row, addrs) {
		if (*addr_row && esix_ip6_addr_match(&hdr->dst_addr, &(*addr_row)->addr)) {
			ip6_forward_payload(hdr->next_header, hdr + 1, ntoh16(hdr->payload_len), hdr);
			break;
		}
	}
}

static int ip6_next_hop_is_destination(struct esix_route_table_row *route) {
	return esix_ip6_addr_match(&route->next_hop, &ip6_addr_null);
}

static struct esix_route_table_row *ip6_get_route(const esix_ip6_addr *dst_addr)
{
	struct esix_route_table_row *matched_route = NULL;
	struct esix_route_table_row **route;

	esix_foreach(route, routes) {		
		if (*route && esix_ip6_addr_match_with_mask(dst_addr, &(*route)->mask, &(*route)->addr)) {
			matched_route = *route;
			break;
		}
	}
	return matched_route;
} 

void esix_ip6_send(const esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr, const uint8_t hlimit, const esix_ip6_next next, const void *payload, int len)
{
	esix_ip6_hdr *hdr;
	int i = -1;
	int dest_onlink;
	struct esix_route_table_row *route = NULL;
	
	hdr = malloc(sizeof(esix_ip6_hdr) + len);
	if (!hdr) {
		return;
	}
	
	hdr->ver_tc_flowlabel = hton32(ip6_hdr_set_version(6));
	hdr->payload_len = hton16(len);
	hdr->next_header = next;
	hdr->hlimit = hlimit;
	hdr->src_addr = ip6_addr_ntoh(src_addr);
	hdr->dst_addr = ip6_addr_ntoh(dst_addr);
	esix_memcpy(hdr + 1, payload, len);

	route = ip6_get_route(dst_addr);
	if (!route) {
		free(hdr);
		return;
	}

	if (ip6_next_hop_is_destination(route)) {
		dest_onlink	= 1;

		if (ip6_addr_is_multicast(dst_addr)) {
			esix_eth_addr lla = ip6_addr_to_eth_multicast(dst_addr);
			esix_eth_send(&lla, esix_eth_type_ip6, hdr, len + sizeof(esix_ip6_hdr));
			return;
		}
		else {
			//it must be unicast, use the neighbor table.
			i = esix_intf_get_neighbor_index(dst_addr, INTERFACE);
		}
	}
	else {
		//use a router
		dest_onlink	= 0;
		i = esix_intf_get_neighbor_index(&route->next_hop, INTERFACE);
	}

	if (i >= 0) {
		//is it reachable?
		if (neighbors[i]->flags.status == ND_REACHABLE ||
			neighbors[i]->flags.status == ND_STALE) {
			//packet leaves here.
			esix_eth_send(&neighbors[i]->lla, esix_eth_type_ip6, hdr, len + sizeof(esix_ip6_hdr));
		}
		else {
			free(hdr);
			return;
		}
	}
	else {
		// we have to send a neighbor solicitation
		if ((i=esix_intf_get_type_address(LINK_LOCAL)) >= 0) {
			if (dest_onlink) {
				esix_icmp6_send_neighbor_sol(&addrs[i]->addr, dst_addr);
			}
			else {
				esix_icmp6_send_neighbor_sol(&addrs[i]->addr, &route->next_hop);
			}
		}
		free(hdr);
	}
	return;
}

uint16_t esix_ip6_upper_checksum(const esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr, const esix_ip6_next next, const void *payload, int len)
{
	uint32_t sum = 0;
	uint16_t const *data;
	uint16_t *src_addr16 = (uint16_t *)src_addr->raw;
	uint16_t *dst_addr16 = (uint16_t *)dst_addr->raw;

	// IPv6 pseudo-header sum : src_addr, dst_addr, type and payload length
	for (data = src_addr16; data < src_addr16+1; data++) {
		sum += *data;
	}
	for (data = dst_addr16; data < dst_addr16+1; data++) {
		sum += *data;
	}
	sum += hton16(len);
	sum += hton16(next);

	// payload sum
	for (data = payload; len > 1; len -= 2) {
		sum += *data++;
	}
	if (len) {
		sum += *((uint8_t *) data);
	}

	while(sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	
	return (uint16_t) ~sum;
}

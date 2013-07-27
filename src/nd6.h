/**
 * @file
 * Comment
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

#ifndef _nd6_H
#define _nd6_H

#include "tools.h"
#include "ip6.h"
#include "eth.h"
#include "config.h"
#include "esix.h"
#include <esix.h>
	
/**
 *  Interface address entry.
 */
typedef struct {
	esix_ip6_addr addr;
	uint8_t masklen;
	uint32_t expiration_date;
	uint32_t preferred_exp_date;
	esix_ip6_addr_type type;
	esix_list link;
} esix_nd6_addr;

/**
 * Route entry.
 */
typedef struct {
	esix_ip6_addr addr;
	esix_ip6_addr mask;
	esix_ip6_addr next_hop;
	uint32_t expiration_date;
	uint8_t ttl;
	uint32_t mtu;
	esix_list link;
} esix_nd6_route;

/**
 * Neighbor status.
 */
typedef enum {
	esix_nd6_nd_status_reachable,
	esix_nd6_nd_status_stale,
	esix_nd6_nd_status_delay,
	esix_nd6_nd_status_unreachable
} esix_nd6_nd_status;

/**
 * Neighbor entry.
 */
typedef struct {
	esix_ip6_addr addr;
	esix_eth_addr lla;
	uint32_t expiration_date;
	uint8_t is_sollicited;
	esix_nd6_nd_status status;
	esix_list link;
} esix_nd6_neighbor;

/**
 * Get the interface ethernet address.
 *
 * @return Interface's ethernet address.
 */
esix_eth_addr esix_nd6_lla(void);

/**
 * Initialize the interface.
 */
esix_err esix_nd6_init(esix_lla lla);

/**
 * Autoconfigure the interface.
 * Add a link local address/route based on the MAC address and join the all-nodes mcast group.
 *
 * @param addr The ethernet address of the interface.
 */
esix_err esix_nd6_autoconfigure(esix_eth_addr addr);

/**
 * Bind an IP address to the interface.
 * Update or add the given address to the assigned addresses table.
 *
 * @param addr            An ipv6 address.
 * @param masklen         Mask length in bits.
 * @param expiration_date Expiration date.
 * @param type            Address type.
 */
esix_err esix_nd6_add_addr(const esix_ip6_addr *addr, uint8_t masklen, uint32_t expiration_date, esix_ip6_addr_type type);
void esix_nd6_remove_addr(const esix_ip6_addr *addr, esix_ip6_addr_type type, uint8_t masklen);
esix_nd6_addr *esix_nd6_get_addr(const esix_ip6_addr *addr, esix_ip6_addr_type type, uint8_t masklen);
esix_nd6_addr *esix_nd6_get_addr_for_type(esix_ip6_addr_type type);

/**
 * Add IP address to the list of neighbors.
 *
 * @param ip_addr         The ipv6 address.
 * @param eth_addr        The corresponding ethernet address.
 * @param expiration_date Expiration date.
 */
esix_err esix_nd6_add_neighbor(const esix_ip6_addr *ip_addr, const esix_eth_addr *eth_addr, uint32_t expiration_date);

/**
 * Remove a neighbor from the cache.
 */
void esix_nd6_remove_neighbor(const esix_ip6_addr *addr);
/*
 * Get the neighbor entry for a given address.
 */
esix_nd6_neighbor *esix_nd6_get_neighbor(const esix_ip6_addr *addr);
esix_nd6_addr *esix_nd6_pick_src_addr(const esix_ip6_addr *dst_addr);
/*
 * esix_nd6_check_source_addr : make sure that the source address isn't multicast
 * if it is, choose an address from the corresponding scope
 */
int esix_nd6_check_source_addr(esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr);

esix_err esix_nd6_add_route(const esix_ip6_addr *addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop, uint32_t expiration_date, uint8_t ttl, uint32_t mtu);
void esix_nd6_remove_route(const esix_ip6_addr *addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop);
esix_nd6_route *esix_nd6_get_route(const esix_ip6_addr *addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop);
esix_nd6_route *esix_nd6_get_route_for_addr(const esix_ip6_addr *addr);

#endif

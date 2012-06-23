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

#ifndef _INTF_H
#define _INTF_H

#include "tools.h"

#include "ip6.h"
#include "eth.h"
#include "config.h"
#include "include/esix.h"


#define ND_SOLLICITED 	1
#define ND_UNSOLLICITED 0
#define	ND_REACHABLE	0
#define ND_STALE	1
#define ND_DELAY	2
#define ND_UNREACHABLE	3

/*
 * Link-layer address (48 bits).
 */
typedef uint16_t esix_ll_addr[3];
	
/**
 * IP address table entry
 */
struct esix_ipaddr_table_row {
	struct ip6_addr addr;	//Actual ip address
	uint8_t	mask;
	uint32_t	expiration_date;//date at which this entry expires.
				//0 : never expires (for now)
	//uint32_t	preferred_exp_date;//date at which this address shouldn't be used if possible
	enum	type type;		//address type : multicast, global unicast, etc...

};

/**
 * Route table entry
 */
struct esix_route_table_row {
	struct 	ip6_addr addr;	//Network address
	struct 	ip6_addr mask;	//netmask
	struct 	ip6_addr next_hop;	//next hop address (should be link-local)
	uint32_t	expiration_date;
	uint8_t	ttl;		//TTL for this route (per-router TTL values are learnt
				//from router advertisements)
	uint32_t	mtu;		//MTU for this route
	uint8_t 	interface;	//interface index
};

struct nb_flags {
	unsigned sollicited : 1; //1 if we sent a neighbor 
	unsigned status	    : 3; //0 = REACHABLE, 1 = STALE, 2 = DELAY, 3 = UNREACHABLE
};

struct esix_neighbor_table_row {
	struct ip6_addr addr;
	esix_ll_addr lla;
	uint32_t	expiration_date;
	uint8_t 	interface;
	struct nb_flags flags;
};


//this table contains every ip address assigned to the system.
struct esix_ipaddr_table_row *addrs[ESIX_MAX_IPADDR];

//this table contains every routes assigned to the system
struct esix_route_table_row *routes[ESIX_MAX_RT];

//table of the neighbors
struct esix_neighbor_table_row *neighbors[ESIX_MAX_NB];

esix_eth_addr *esix_intf_get_lla(void);
void esix_intf_init_interface(esix_eth_addr, uint8_t);
void esix_intf_add_default_neighbors(esix_ll_addr);
int esix_intf_add_neighbor_row(struct esix_neighbor_table_row *row);
int esix_intf_add_neighbor(const struct ip6_addr *, esix_ll_addr, uint32_t, uint8_t);
int esix_intf_get_neighbor_index(const struct ip6_addr *, uint8_t);
int esix_intf_pick_source_address(const struct ip6_addr *);

void esix_intf_add_default_addresses(void);
int esix_intf_add_address_row(struct esix_ipaddr_table_row *row);
int esix_intf_add_address(struct ip6_addr *, uint8_t, uint32_t, enum type);
int esix_intf_remove_address(const struct ip6_addr *, enum type, uint8_t);
int esix_intf_get_address_index(const struct ip6_addr *, enum type, uint8_t);
int esix_intf_get_type_address(enum type);

void esix_intf_add_default_routes(uint8_t intf_index, int intf_mtu);	
int esix_intf_add_route_row(struct esix_route_table_row *row);
int esix_intf_add_route(struct ip6_addr *, struct ip6_addr *, struct ip6_addr *, uint32_t, uint8_t, uint32_t, uint8_t);
int esix_intf_check_source_addr(struct ip6_addr *, const struct ip6_addr *);
int esix_intf_get_route_index(const struct ip6_addr *, const struct ip6_addr *, const struct ip6_addr *, const uint8_t);
int esix_intf_remove_neighbor(const struct ip6_addr *, uint8_t);
int esix_intf_remove_route(struct ip6_addr *, struct ip6_addr *, struct ip6_addr *, uint8_t);
//int esix_intf_get_route_index(struct ip6_addr *, struct ip6_addr *, struct ip6_addr*, uint8_t);


#endif

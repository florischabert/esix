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

#include "ip6.h"
#include "intf.h"
#include "config.h"
#include "include/esix.h"

//general purpose buffer, mainly used to build packets/
//static int esix_buffer[ESIX_BUFFER_SIZE];

/**
 * IP address table entry
 */
struct esix_ipaddr_table_row {
	struct ip6_addr addr;	//Actual ip address
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

struct esix_neighbor_table_row {
	struct ip6_addr addr;
	struct esix_mac_addr mac_addr;
	u32_t	expiration_date;
	u8_t interface;
};


//this table contains every ip address assigned to the system.
struct esix_ipaddr_table_row *addrs[ESIX_MAX_IPADDR];

//this table contains every routes assigned to the system
struct esix_route_table_row *routes[ESIX_MAX_RT];

//table of the neighbors
struct esix_neighbor_table_row *neighbors[ESIX_MAX_NB];


void esix_intf_add_default_neighbors(struct esix_mac_addr addr);	

/**
 * Adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void esix_intf_add_default_addresses(void);	

void esix_intf_add_default_routes(int intf_index, int intf_mtu);	

int esix_intf_add_neighbor_row(struct esix_neighbor_table_row *row);

int esix_intf_add_neighbor(u32_t, u32_t, u32_t, u32_t, struct esix_mac_addr, u32_t, u8_t);

/**
 * Adds the given IP address to the table.
 *
 * @return 1 on success.
 */
int esix_intf_add_address_row(struct esix_ipaddr_table_row *row);

/**
 * esix_add_to_active_routes : adds the given route to the routing table. Returns 1 on success.
 */
int esix_intf_add_route_row(struct esix_route_table_row *row);

/**
 * esix_new_addr : creates an addres with the passed arguments
 * and adds or updates it.
 */
int esix_intf_add_address(u32_t, u32_t, u32_t, u32_t, u8_t, u32_t, int);

int esix_intf_remove_address(u8_t, u32_t, u32_t, u32_t, u32_t, u8_t);


int esix_intf_add_route(u32_t, u32_t, u32_t, u32_t, u8_t, u32_t, u32_t, u32_t, 
	u32_t, u32_t, u8_t, u16_t, u8_t);

#endif

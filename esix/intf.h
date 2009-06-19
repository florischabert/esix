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

#include "intf.h"
#include "config.h"
#include "include/esix.h"

//general purpose buffer, mainly used to build packets/
//static int esix_buffer[ESIX_BUFFER_SIZE];

//this table contains every ip address assigned to the system.
struct esix_ipaddr_table_row *addrs[ESIX_MAX_IPADDR];

//this table contains every routes assigned to the system
struct esix_route_table_row *routes[ESIX_MAX_RT];

//stores our mac addr, for now...
struct esix_mac_addr mac_addr;

/**
 * Adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void esix_intf_add_basic_addr_routes(struct esix_mac_addr addr, int intf_index, int intf_mtu);	

/**
 * Adds the given IP address to the table.
 *
 * @return 1 on success.
 */
int esix_intf_add_to_active_addresses(struct esix_ipaddr_table_row *row);

/**
 * esix_add_to_active_routes : adds the given route to the routing table. Returns 1 on success.
 */
int esix_intf_add_to_active_routes(struct esix_route_table_row *row);

/**
 * esix_new_addr : creates an addres with the passed arguments
 * and adds or updates it.
 */
int esix_intf_new_addr(u32_t, u32_t, u32_t, u32_t, u8_t, u32_t, int);

int esix_intf_remove_addr(u8_t, u32_t, u32_t, u32_t, u32_t, u8_t);


int esix_intf_new_route(u32_t, u32_t, u32_t, u32_t, u8_t, u32_t, u32_t, u32_t, 
	u32_t, u32_t, u8_t, u16_t, u8_t);

#endif

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

#include "intf.h"
#include "tools.h"
#include "ip6.h"
#include "include/esix.h"

/**
 * Adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void esix_intf_add_basic_addr_routes(struct esix_mac_addr addr, int intf_index, int intf_mtu)
{
	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.

	//unicast link local
	esix_intf_new_addr(	hton32(0xfe800000), 	//0xfe80 
			hton32(0x00000000),
			hton32((addr.h << 16) | ((addr.l >> 16) & 0xff00) | 0xff),	//stateless autoconf FIXME: universal bit ?
			hton32((0xfe << 24) | (addr.l & 0xffffff)),
			0x80,			// /128
			0x0,			//this one never expires
			LINK_LOCAL_SCOPE);


	//multicastsolicited-node address (for neighbor discovery)
	esix_intf_new_addr(	hton32(0xff020000), 	//0xff02 
			hton32(0x00000000),	//0x0000
			hton32(0x00000001),	//0x0001
			hton32((0xff << 24) | (addr.l & 0xffffff)),
			0x80,			// /128
			0x0,			//this one never expires
			MCAST_SCOPE);


	//multicast all-nodes (for router advertisements)
	esix_intf_new_addr(	hton32(0xff020000), 	//ff02::1 
			hton32(0x00000000), 
			hton32(0x00000000),
			hton32(0x00000001),
			0x80, 			// /128
			0x0,			//this one never expires
			MCAST_SCOPE);


	//link local route (fe80::/64)
	struct esix_route_table_row *ll_rt	= esix_w_malloc(sizeof(struct esix_route_table_row));
	
	ll_rt->addr.addr1	= hton32(0xfe800000);
	ll_rt->addr.addr2	= 0x0;
	ll_rt->addr.addr3	= 0x0;
	ll_rt->addr.addr4	= 0x0;
	ll_rt->mask		= 64;
	ll_rt->next_hop.addr1	= 0x0; //a value of 0 means no next hop 
	ll_rt->next_hop.addr2	= 0x0; 
	ll_rt->next_hop.addr3	= 0x0; 
	ll_rt->next_hop.addr4	= 0x0; 
	ll_rt->ttl		= DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	ll_rt->mtu		= intf_mtu;	
	ll_rt->expiration_date	= 0x0; //this never expires
	ll_rt->interface	= intf_index;

	//multicast route (ff00:: /8)
	struct esix_route_table_row *mcast_rt	= esix_w_malloc(sizeof(struct esix_route_table_row));
	
	mcast_rt->addr.addr1		= hton32(0xff000000);
	mcast_rt->addr.addr2		= 0x0;
	mcast_rt->addr.addr3		= 0x0;
	mcast_rt->addr.addr4		= 0x0;
	mcast_rt->mask			= 8;
	mcast_rt->next_hop.addr1	= 0x0; //a value of 0 means no next hop 
	mcast_rt->next_hop.addr2	= 0x0; 
	mcast_rt->next_hop.addr3	= 0x0; 
	mcast_rt->next_hop.addr4	= 0x0; 
	mcast_rt->expiration_date	= 0x0; //this never expires
	mcast_rt->ttl			= DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	mcast_rt->mtu			= intf_mtu;
	mcast_rt->interface		= intf_index;

	esix_intf_add_to_active_routes(ll_rt);
	esix_intf_add_to_active_routes(mcast_rt);
}

/**
 * Adds the given IP address to the table.
 *
 * @return 1 on success.
 */
int esix_intf_add_to_active_addresses(struct esix_ipaddr_table_row *row)
{
	int i=0;

	//look for an empty place where to store our entry.
	while(i<ESIX_MAX_IPADDR)
	{
		if(addrs[i] == NULL)
		{
			addrs[i] = row;
			return 1;
		}
		i++;
	}
	//sorry dude, table was full.
	return 0;
}

/**
 * esix_add_to_active_routes : adds the given route to the routing table. Returns 1 on success.
 */
int esix_intf_add_to_active_routes(struct esix_route_table_row *row)
{
	int i=0;

	while(i<ESIX_MAX_RT)
	{
		if(routes[i]	== NULL)
		{
			routes[i] = row;
			return 1;
		}
		i++;
	}	
	//sorry dude, table was full.
	return 0;
}

/**
 * esix_new_addr : creates an addres with the passed arguments
 * and adds or updates it.
 */
int esix_intf_new_addr(u32_t addr1, u32_t addr2, u32_t addr3, u32_t addr4, u8_t masklen,
			u32_t expiration_date, int scope)
{
	struct esix_ipaddr_table_row *new_addr = 
				esix_w_malloc(sizeof (struct esix_ipaddr_table_row)); 

	new_addr->addr.addr1		= addr1;
	new_addr->addr.addr2		= addr2;
	new_addr->addr.addr3		= addr3;
	new_addr->addr.addr4		= addr4;
	new_addr->expiration_date	= expiration_date;
	new_addr->scope			= scope;
	new_addr->mask			= masklen;

	int j=0;
	while(j<ESIX_MAX_IPADDR)
	{
		//check if we already stored this address
		if((addrs[j] != NULL) &&
			(addrs[j]->scope	== new_addr->scope)	 &&
			(addrs[j]->expiration_date != 0) 		 &&
			(addrs[j]->addr.addr1	== new_addr->addr.addr1) &&
			(addrs[j]->addr.addr2	== new_addr->addr.addr2) &&
			(addrs[j]->addr.addr3	== new_addr->addr.addr3) &&
			(addrs[j]->addr.addr4	== new_addr->addr.addr4) &&
			(addrs[j]->mask		== new_addr->mask ))
		{
			//we're only updating it
			addrs[j]->expiration_date = 
				new_addr->expiration_date;
			esix_w_free(new_addr);
			return 1;
		}
		j++;
	}//while(j<...
	//TODO perform DAD
	//
	//it's new, perform DAD and 
	//try to add it to the table of addresses and
	//free it so we don't leak memory in case it fails
	if(esix_intf_add_to_active_addresses(new_addr))
		return 1;

	//if we're still here, something's wrong.
	esix_w_free(new_addr);
	return 0;
}

int esix_intf_remove_addr(u8_t scope, u32_t addr1, u32_t addr2, u32_t addr3, u32_t addr4, u8_t masklen)
{
	int j=0;
	struct esix_ipaddr_table_row *ptr;
	while(j<ESIX_MAX_IPADDR)
	{
		//find'em and detroy'em :)
		if((addrs[j] != NULL) &&
			(addrs[j]->scope	== scope) &&
			(addrs[j]->addr.addr1	== addr1) &&
			(addrs[j]->addr.addr2	== addr2) &&
			(addrs[j]->addr.addr3	== addr3) &&
			(addrs[j]->addr.addr4	== addr4) &&
			(addrs[j]->mask		== masklen))
		{
			//first remove the entry from the table
			//then free it's allocated memory
			//might avoid random crashes
			ptr		= addrs[j];
			addrs[j] 	= NULL; 
			esix_w_free(ptr);
			return 1;
		}
	}
	return 0;
}

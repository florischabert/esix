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

void esix_intf_add_default_neighbors(struct esix_mac_addr maddr)
{
	//first row of the neighbors table is us
	esix_intf_add_neighbor(0,0,0,hton32(1), // ip address FIXME: which one ?
	                       maddr, // MAC address
	                       0, // never expires
	                       INTERFACE);
}

/**
 * Adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void esix_intf_add_default_addresses(void)
{
	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.

	//unicast link local
	esix_intf_add_address(	hton32(0xfe800000), 	//0xfe80 
			hton32(0x00000000),
			hton32(neighbors[0]->mac_addr.l | 0x020000ff),	//stateless autoconf, 0x02 : universal bit
			//0xfe here is OK
			hton32((0xfe000000) | ((neighbors[0]->mac_addr.l << 16) & 0xff0000) | neighbors[0]->mac_addr.h),
			0x80,			// /128
			0x0,			//this one never expires
			LINK_LOCAL_SCOPE);

	//multicast solicited-node address (for neighbor discovery)
	esix_intf_add_address(	hton32(0xff020000), 	//0xff02 
			hton32(0x00000000),	//0x0000
			hton32(0x00000001),	//0x0001
			//0xff here is OK
			hton32((0xff000000) | ((neighbors[0]->mac_addr.l << 16) & 0xff0000) | neighbors[0]->mac_addr.h),
			0x80,			// /128
			0x0,			//this one never expires
			MCAST_SCOPE);


	//multicast all-nodes (for router advertisements)
	esix_intf_add_address(	hton32(0xff020000), 	//ff02::1 
			hton32(0x00000000), 
			hton32(0x00000000),
			hton32(0x00000001),
			0x80, 			// /128
			0x0,			//this one never expires
			MCAST_SCOPE);
}

void esix_intf_add_default_routes(int intf_index, int intf_mtu)
{
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

	esix_intf_add_route_row(ll_rt);
	esix_intf_add_route_row(mcast_rt);
}

int esix_intf_add_neighbor_row(struct esix_neighbor_table_row *row)
{
	int i=0;
	
	//look for an empty place where to store our entry.
	while(i<ESIX_MAX_NB)
	{
		if(neighbors[i] == NULL)
		{
			neighbors[i] = row;
			return 1;
		}
		i++;
	}
	//sorry dude, table was full.
	return 0;
}

int esix_intf_add_neighbor(u32_t addr1, u32_t addr2, u32_t addr3, u32_t addr4, struct esix_mac_addr mac_addr, u32_t expiration_date, u8_t interface)
{
	int i = 0;
	struct esix_neighbor_table_row *nb;
	
	//try to look up this neighbor in the table
	//to find if it already exists and only needs an update
	while(i<ESIX_MAX_NB)
	{
		if((neighbors[i] != NULL) &&
			(neighbors[i]->addr.addr1		== addr1) &&
			(neighbors[i]->addr.addr2		== addr2) &&
			(neighbors[i]->addr.addr3		== addr3) &&
			(neighbors[i]->addr.addr4		== addr4) &&
			(neighbors[i]->interface		== interface))
		{
			//we found something, just update some variables
				nb	= neighbors[i];
				nb->expiration_date	= expiration_date;
				nb->mac_addr			= mac_addr;
				return 1;
		}
		i++;
	}

	//we're still there, let's create the new neighbor.
	nb	= esix_w_malloc(sizeof(struct esix_neighbor_table_row));

	//hmmmm... I smell gas...
	if(nb == NULL)
		return 0;

	nb->addr.addr1			= addr1;
	nb->addr.addr2			= addr2;
	nb->addr.addr3	 		= addr3;
	nb->addr.addr4			= addr4;
	nb->expiration_date	= expiration_date;
	nb->mac_addr			= mac_addr;
	nb->interface			= interface;
	
	//now try to add it
	if(esix_intf_add_neighbor_row(nb))
		return 1;

	//we're still here, clean our mess up.
	esix_w_free(nb);
	return 0;
}

/**
 * Adds the given IP address to the table.
 *
 * @return 1 on success.
 */
int esix_intf_add_address_row(struct esix_ipaddr_table_row *row)
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
 * esix_add_to_active_routes : adds the given route to the routing table. 
 *
 * @return 1 on success.
 */
int esix_intf_add_route_row(struct esix_route_table_row *row)
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
int esix_intf_add_address(u32_t addr1, u32_t addr2, u32_t addr3, u32_t addr4, u8_t masklen,
			u32_t expiration_date, int scope)
{
	struct esix_ipaddr_table_row *new_addr;

	int j=0;
	while(j<ESIX_MAX_IPADDR)
	{
		//check if we already stored this address
		if((addrs[j] != NULL) &&
			(addrs[j]->scope	== scope) &&
			(addrs[j]->expiration_date != 0)  &&
			(addrs[j]->addr.addr1	== addr1) &&
			(addrs[j]->addr.addr2	== addr2) &&
			(addrs[j]->addr.addr3	== addr3) &&
			(addrs[j]->addr.addr4	== addr4) &&
			(addrs[j]->mask		== masklen ))
		{
			//we're only updating it's lifetime.
			addrs[j]->expiration_date = expiration_date;
			return 1;
		}
		j++;
	}

	//we're still there, let's create our new address.
	new_addr = esix_w_malloc(sizeof (struct esix_ipaddr_table_row)); 

	//hmmmm... I smell gas...
	if(new_addr == NULL)
		return 0;

	new_addr->addr.addr1		= addr1;
	new_addr->addr.addr2		= addr2;
	new_addr->addr.addr3		= addr3;
	new_addr->addr.addr4		= addr4;
	new_addr->expiration_date	= expiration_date;
	new_addr->scope			= scope;
	new_addr->mask			= masklen;

	//TODO perform DAD
	//
	//it's new, perform DAD and 
	//try to add it to the table of addresses and
	//free it so we don't leak memory in case it fails
	if(esix_intf_add_address_row(new_addr))
		return 1;

	//if we're still here, something went wrong.
	esix_w_free(new_addr);
	return 0;
}

int esix_intf_remove_address(u8_t scope, u32_t addr1, u32_t addr2, u32_t addr3, u32_t addr4, u8_t masklen)
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
		j++;
	}
	return 0;
}

int esix_intf_add_route(u32_t dst1, u32_t dst2, u32_t dst3, u32_t dst4, u8_t mask, u32_t nxt_hop1,
				u32_t nxt_hop2, u32_t nxt_hop3, u32_t nxt_hop4, u32_t expiration_date,
				u8_t ttl, u16_t mtu, u8_t interface)
{
	int i=0;
	struct esix_route_table_row *rt	= NULL;

	//try to look up this route in the table
	//to find if it already exists and only needs an update
	while(i<ESIX_MAX_RT)
	{
		if((routes[i] != NULL) &&
			(routes[i]->addr.addr1		== dst1) &&
			(routes[i]->addr.addr2		== dst2) &&
			(routes[i]->addr.addr3		== dst3) &&
			(routes[i]->addr.addr4		== dst4) &&
			(routes[i]->next_hop.addr1	== nxt_hop1) &&
			(routes[i]->next_hop.addr2	== nxt_hop2) &&
			(routes[i]->next_hop.addr3	== nxt_hop3) &&
			(routes[i]->next_hop.addr4	== nxt_hop4) &&
			(routes[i]->mask		== mask)     &&
			(routes[i]->interface		== interface))
			
			{
			//we found something, just update some variables
				rt	= routes[i];
				rt->expiration_date	= expiration_date;
				rt->ttl			= ttl;
				rt->mtu			= mtu;
				return 1;
			}
		i++;
	}

	//we're still there, let's create the new route.
	rt	= esix_w_malloc(sizeof(struct esix_route_table_row));

	//hmmmm... I smell gas...
	if(rt == NULL)
		return 0;

	rt->addr.addr1		= dst1;
	rt->addr.addr2		= dst2;
	rt->addr.addr3		= dst3;
	rt->addr.addr4		= dst4;
	rt->mask		= mask;
	rt->next_hop.addr1	= nxt_hop1;
	rt->next_hop.addr2	= nxt_hop2;
	rt->next_hop.addr3	= nxt_hop3;
	rt->next_hop.addr4	= nxt_hop4;
	rt->expiration_date	= expiration_date;
	rt->ttl			= ttl;
	rt->mtu			= mtu;
	rt->interface		= interface;

	//now try to add it
	if(esix_intf_add_route_row(rt))
		return 1;

	//we're still here, clean our mess up.
	esix_w_free(rt);
	return 0;
} 

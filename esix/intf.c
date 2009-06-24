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

void esix_intf_add_default_neighbors(esix_ll_addr lla)
{
	struct ip6_addr addr;
	
	//first row of the neighbors table is us
	addr.addr1 = 0;
	addr.addr2 = 0;
	addr.addr3 = 0;
	addr.addr4 = hton32(1);
	esix_intf_add_neighbor(&addr, // ip address FIXME: which one ?
		lla, // MAC address
		0, // never expires
		INTERFACE);
}

/**
 * Adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void esix_intf_add_default_addresses(void)
{
	struct ip6_addr addr;
	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.
	
	//unicast link local
	addr.addr1 = 	hton32(0xfe800000); //0xfe80 
	addr.addr2 = 	hton32(0x00000000);
	addr.addr3 = 	hton32(	(ntoh16(neighbors[0]->lla[0]) << 16 & 0xff0000)
				| (ntoh16(neighbors[0]->lla[1]) & 0xff00)
				| 0x020000ff ); //stateless autoconf, 0x02 : universal bit

	addr.addr4 = 	hton32(	(0xfe000000) //0xfe here is OK
				| (ntoh16(neighbors[0]->lla[1])<< 16 & 0xff0000)
				| (ntoh16(neighbors[0]->lla[2])) );

	esix_intf_add_address(&addr,
			0x80,			// /128
			0x0,			//this one never expires
			LINK_LOCAL_SCOPE);

	//multicast solicited-node address (for neighbor discovery)
	addr.addr1 = 	hton32(0xff020000); 	//0xff02 
	addr.addr2 = 	0;
	addr.addr3 = 	hton32(1);
	addr.addr4 = 	hton32(	(0xff000000)
				| (ntoh16(neighbors[0]->lla[1]) << 16 & 0xff0000)
				| (ntoh16(neighbors[0]->lla[2])) );

	esix_intf_add_address(&addr,
			0x80,			// /128
			0x0,			//this one never expires
			MCAST_SCOPE);


	//multicast all-nodes (for router advertisements)
	addr.addr1 = 	hton32(0xff020000); 	//ff02::1 
	addr.addr2 = 	0;
	addr.addr3 = 	0;
	addr.addr4 = 	hton32(1);
	esix_intf_add_address(&addr,
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

int esix_intf_add_neighbor(struct ip6_addr *addr, esix_ll_addr lla, u32_t expiration_date, u8_t interface)
{
	int j, i = 0;
	struct esix_neighbor_table_row *nb;

	i = esix_intf_get_neighbor_index(addr, interface);
	if(i >= 0)
	{
		neighbors[i]->expiration_date	= expiration_date;
		for(j = 0; j < 3; j++)
			neighbors[i]->lla[j] = lla[j];
		return 1;
	}

	//we're still there, let's create the new neighbor.
	nb	= esix_w_malloc(sizeof(struct esix_neighbor_table_row));

	//hmmmm... I smell gas...
	if(nb == NULL)
		return 0;

	nb->addr.addr1			= addr->addr1;
	nb->addr.addr2			= addr->addr2;
	nb->addr.addr3	 		= addr->addr3;
	nb->addr.addr4			= addr->addr4;
	nb->expiration_date	= expiration_date;
	for(j = 0; j < 3; j++)
		nb->lla[j] = lla[j];
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

/*
 * Return the neighbor row index of the given address.
 */
int esix_intf_get_neighbor_index(struct ip6_addr *addr, u8_t interface)
{
	int i = 0;

	//try to look up this neighbor in the table
	//to find if it already exists and only needs an update
	while(i<ESIX_MAX_NB)
	{
		if((neighbors[i] != NULL) &&
			(neighbors[i]->addr.addr1		== addr->addr1) &&
			(neighbors[i]->addr.addr2		== addr->addr2) &&
			(neighbors[i]->addr.addr3		== addr->addr3) &&
			(neighbors[i]->addr.addr4		== addr->addr4) &&
			(neighbors[i]->interface		== interface))
		{
			//we found something, just update some variables
			return i;
		}
		i++;
	}
	return -1;
}

/*
 * Returns any address of specified scope.
 */
int esix_intf_get_scope_address(u8_t scope)
{
	int i;
	for(i=0; i<ESIX_MAX_IPADDR;i++)
	{
		if( (addrs[i] != NULL) &&
			((addrs[i]->scope == scope) || (scope = ANY_SCOPE)) )
			return i;
	}

	return -1;
}

/*
 * Return the address row index of the given address.
 */
int esix_intf_get_address_index(struct ip6_addr *addr, u8_t scope, u8_t masklen)
{
	int j;
	for(j = 0; j<ESIX_MAX_IPADDR; j++)
	{
		//check if we already stored this address
		if((addrs[j] != NULL) &&
			((addrs[j]->scope	== scope) || (scope == ANY_SCOPE)) &&
			(addrs[j]->addr.addr1 == addr->addr1) &&
			(addrs[j]->addr.addr2 == addr->addr2) &&
			(addrs[j]->addr.addr3 == addr->addr3) &&
			(addrs[j]->addr.addr4 == addr->addr4) &&
			((addrs[j]->mask == masklen) || (masklen == ANY_MASK)))
		{
			return j;
		}
	}
	return -1;
}

/**
 * esix_new_addr : creates an addres with the passed arguments
 * and adds or updates it.
 */
int esix_intf_add_address(struct ip6_addr *addr, u8_t masklen, u32_t expiration_date, u8_t scope)
{
	struct esix_ipaddr_table_row *row;
	int i;

	i = esix_intf_get_address_index(addr, scope, masklen);
	if(i >= 0)
	{
		//we're only updating it's lifetime.
		addrs[i]->expiration_date = expiration_date;
		return 1;
	}

	//we're still there, let's create our new address.
	row = esix_w_malloc(sizeof(struct esix_ipaddr_table_row)); 

	//hmmmm... I smell gas...
	if(row == NULL)
		return 0;

	row->addr.addr1		= addr->addr1;
	row->addr.addr2		= addr->addr2;
	row->addr.addr3		= addr->addr3;
	row->addr.addr4		= addr->addr4;
	row->expiration_date	= expiration_date;
	row->scope			= scope;
	row->mask			= masklen;

	//TODO perform DAD
	//
	//it's new, perform DAD and 
	//try to add it to the table of addresses and
	//free it so we don't leak memory in case it fails
	if(esix_intf_add_address_row(row))
		return 1;

	//if we're still here, something went wrong.
	esix_w_free(row);
	return 0;
}

int esix_intf_remove_address(struct ip6_addr *addr, u8_t scope, u8_t masklen)
{
	int i;
	struct esix_ipaddr_table_row *row;
	
	i = esix_intf_get_address_index(addr, scope, masklen);
	if(i >= 0)
	{
		row = addrs[i];
		addrs[i] = NULL; 
		esix_w_free(row);
		return 1;
	}

	return 0;
}

int esix_intf_add_route(struct ip6_addr *daddr, u8_t mask, struct ip6_addr *next_addr, u32_t expiration_date,
				u8_t ttl, u32_t mtu, u8_t interface)
{
	int i=0;
	struct esix_route_table_row *rt	= NULL;

	//try to look up this route in the table
	//to find if it already exists and only needs an update
	while(i<ESIX_MAX_RT)
	{
		if((routes[i] != NULL) &&
			(routes[i]->addr.addr1		== daddr->addr1) &&
			(routes[i]->addr.addr2		== daddr->addr2) &&
			(routes[i]->addr.addr3		== daddr->addr3) &&
			(routes[i]->addr.addr4		== daddr->addr4) &&
			(routes[i]->next_hop.addr1	== next_addr->addr1) &&
			(routes[i]->next_hop.addr2	== next_addr->addr2) &&
			(routes[i]->next_hop.addr3	== next_addr->addr3) &&
			(routes[i]->next_hop.addr4	== next_addr->addr4) &&
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

	rt->addr.addr1		= daddr->addr1;
	rt->addr.addr2		= daddr->addr2;
	rt->addr.addr3		= daddr->addr3;
	rt->addr.addr4		= daddr->addr4;
	rt->mask		= mask;
	rt->next_hop.addr1	= next_addr->addr1;
	rt->next_hop.addr2	= next_addr->addr2;
	rt->next_hop.addr3	= next_addr->addr3;
	rt->next_hop.addr4	= next_addr->addr4;
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

/*
 * esix_intf_check_source_addr : make sure that the source address isn't multicast
 */
int esix_intf_check_source_addr(struct ip6_addr *saddr, struct ip6_addr *daddr)
{
	int i = -1;

	if(	(saddr->addr1 & hton32(0xff000000)) == hton32(0xff000000))
	{
		//try to chose an address of the correct scope to replace it.
		if((daddr->addr1 & hton32(0xffff0000)) == hton32(0xfe800000))
			i = esix_intf_get_scope_address(LINK_LOCAL_SCOPE);
		
		if( (i < 0 ) && (i = esix_intf_get_scope_address(GLOBAL_SCOPE)) < 0)
		{
			toggle_led();
			return -1;
		}
			
			*saddr	= addrs[i]->addr; 
	}
	return 1;
}

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
#include "icmp6.h"
#include "include/esix.h"
#include "eth.h"

esix_eth_addr esix_intf_lla;

/**
 * Adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void esix_intf_init_interface(esix_eth_addr lla, uint8_t  interface)
{
	esix_ip6_addr addr;

	esix_intf_lla = lla;

	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.
	
	//unicast link local
	addr.raw[0] = 	hton32(0xfe800000); //0xfe80 
	addr.raw[1] = 	hton32(0x00000000);
	addr.raw[2] = 	hton32(	(ntoh16(lla.raw[0]) << 16 & 0xff0000)
				| (ntoh16(lla.raw[1]) & 0xff00)
				| 0x020000ff ); //stateless autoconf, 0x02 : universal bit

	addr.raw[3] = 	hton32(	(0xfe000000) //0xfe here is OK
				| (ntoh16(lla.raw[1])<< 16 & 0xff0000)
				| (ntoh16(lla.raw[2])) );

	esix_intf_add_address(&addr,
			0x80,			// /128
			0x0,			//this one never expires
			LINK_LOCAL);

	//first row of the neighbors table is us
        addr.raw[0] =    hton32(0xfe800000); //0xfe80
        addr.raw[1] =    hton32(0x00000000);
        addr.raw[2] =    hton32( (ntoh16(lla.raw[0]) << 16 & 0xff0000)
                                | (ntoh16(lla.raw[1]) & 0xff00)
                                | 0x020000ff ); //stateless autoconf, 0x02 : universal bit

        addr.raw[3] =    hton32( (0xfe000000) //0xfe here is OK
                                | (ntoh16(lla.raw[1])<< 16 & 0xff0000)
                                | (ntoh16(lla.raw[2])) );

	esix_intf_add_neighbor(&addr,
		lla, // MAC address
		0, // never expires
		INTERFACE);

	//multicast all-nodes (for router advertisements)
	addr.raw[0] = 	hton32(0xff020000); 	//ff02::1 
	addr.raw[1] = 	0;
	addr.raw[2] = 	0;
	addr.raw[3] = 	hton32(1);
	esix_intf_add_address(&addr,
			0x80, 			// /128
			0x0,			//this one never expires
			MULTICAST);

}

void esix_intf_add_default_routes(uint8_t intf_index, int intf_mtu)
{
	//link local route (fe80::/64)
	struct esix_route_table_row *ll_rt	= malloc(sizeof(struct esix_route_table_row));
	if(ll_rt == NULL)
		return;
	
	ll_rt->addr.raw[0]	= hton32(0xfe800000);
	ll_rt->addr.raw[1]	= 0x0;
	ll_rt->addr.raw[2]	= 0x0;
	ll_rt->addr.raw[3]	= 0x0;
	ll_rt->mask.raw[0]	= 0xffffffff;
	ll_rt->mask.raw[1]	= 0xffffffff;
	ll_rt->mask.raw[2]	= 0x0;
	ll_rt->mask.raw[3]	= 0x0;
	ll_rt->next_hop.raw[0]	= 0x0; //a value of 0 means no next hop 
	ll_rt->next_hop.raw[1]	= 0x0; 
	ll_rt->next_hop.raw[2]	= 0x0; 
	ll_rt->next_hop.raw[3]	= 0x0; 
	ll_rt->ttl		= DEFAULT_TTL; 
	ll_rt->mtu		= intf_mtu;
	ll_rt->expiration_date	= 0x0; //this never expires
	ll_rt->interface	= intf_index;

	//multicast route (ff00:: /8)
	struct esix_route_table_row *mcast_rt	= malloc(sizeof(struct esix_route_table_row));
	if(mcast_rt == NULL)
	{
		free(ll_rt);
		return;
	}
	
	mcast_rt->addr.raw[0]		= hton32(0xff000000);
	mcast_rt->addr.raw[1]		= 0x0;
	mcast_rt->addr.raw[2]		= 0x0;
	mcast_rt->addr.raw[3]		= 0x0;
	mcast_rt->mask.raw[0]		= hton32(0xff000000); // /8
	mcast_rt->mask.raw[1]		= 0x0;
	mcast_rt->mask.raw[2]		= 0x0;
	mcast_rt->mask.raw[3]		= 0x0;
	mcast_rt->next_hop.raw[0]	= 0x0; //a value of 0 means no next hop 
	mcast_rt->next_hop.raw[1]	= 0x0; 
	mcast_rt->next_hop.raw[2]	= 0x0; 
	mcast_rt->next_hop.raw[3]	= 0x0; 
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

int esix_intf_add_neighbor(const esix_ip6_addr *addr, esix_eth_addr lla, uint32_t expiration_date, uint8_t interface)
{
	int j, i = 0;
	struct esix_neighbor_table_row *nb;

	i = esix_intf_get_neighbor_index(addr, interface);
	if(i >= 0)
	{
		neighbors[i]->expiration_date	= expiration_date;
		for(j = 0; j < 3; j++)
			neighbors[i]->lla.raw[j] = lla.raw[j];
		return 1;
	}

	//we're still here, create the new neighbor.
	nb	= malloc(sizeof(struct esix_neighbor_table_row));

	//hmmmm... I smell gas...
	if(nb == NULL)
		return 0;

	nb->addr.raw[0]			= addr->raw[0];
	nb->addr.raw[1]			= addr->raw[1];
	nb->addr.raw[2]	 		= addr->raw[2];
	nb->addr.raw[3]			= addr->raw[3];
	nb->expiration_date		= expiration_date;
	for(j = 0; j < 3; j++)
		nb->lla.raw[j] = lla.raw[j];
	nb->interface			= interface;
	
	//now try to add it
	if(esix_intf_add_neighbor_row(nb))
		return 1;

	//we're still here, clean our mess up.
	free(nb);
	
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
 *  esix_intf_add_route_row: adds the given route to the routing table. 
 *
 * @return 1 on success.
 */

int esix_intf_add_route_row(struct esix_route_table_row *row)
{
	int i;
	int j=-1;
	int unsorted;

	for(i=0;i<ESIX_MAX_RT;i++)
	{
		if(routes[i]	== NULL)
		{
			routes[i] = row;
			j = i;
			break;
		}
	}	

	//sorry dude, table was full.
	if(j < 0)
		return 0;

/*
	//sort the table to ease the routing process
	//routes are sorted by ascending netmask.

	//FIXME: this definitely looks like hell... (4am code)
	i=0;
	unsorted = 1;
	while(unsorted)
	{
		unsorted = 0;
		for(i=0; i<ESIX_MAX_RT-1; i++)
		{
			if(routes[i] == NULL)
				continue;
			j=i+1;

			while(routes[j] == NULL && j < ESIX_MAX_RT)
				j++;

			if(routes[j] == NULL)
				continue;

			for(k=0; k<4; k++)
			{
				if( *((uint32_t*) (&routes[i]->mask.raw[0])+k) < *((uint32_t*) (&routes[j]->mask.raw[0])+k) )
				{
					unsorted = 1;
					tmp 		= routes[i];
					routes[i]	= routes[j];
					routes[j]	= tmp;
					break;
				}
			}
		}
	}*/
	return 1;
}

/*
 * Return the neighbor row index of the given address.
 */
int esix_intf_get_neighbor_index(const esix_ip6_addr *addr, uint8_t interface)
{
	int i = 0;

	//try to look up this neighbor in the table
	//to find if it already exists and only needs an update
	while(i<ESIX_MAX_NB)
	{
		if((neighbors[i] != NULL) &&
			(neighbors[i]->addr.raw[0]		== addr->raw[0]) &&
			(neighbors[i]->addr.raw[1]		== addr->raw[1]) &&
			(neighbors[i]->addr.raw[2]		== addr->raw[2]) &&
			(neighbors[i]->addr.raw[3]		== addr->raw[3]) &&
			(neighbors[i]->interface		== interface))
		{
			return i;
		}
		i++;
	}
	return -1;
}

/*
 * esix_intf_remove_neighbor : removes a neighbor from the cache.
 */
int esix_intf_remove_neighbor(const esix_ip6_addr *addr, uint8_t interface)
{

	int i;
	struct esix_neighbor_table_row *row;
	
	i = esix_intf_get_neighbor_index(addr, interface);
	if(i >= 0)
	{
		row = neighbors[i];
		neighbors[i] = NULL; 
		free(row);
		return 1;
	}

	return 0;
}

/*
 * Returns any address of specified type.
 */
int esix_intf_get_type_address(enum type type)
{
	int i;
	for(i=0; i<ESIX_MAX_IPADDR;i++)
	{
		if( (addrs[i] != NULL) &&
			((addrs[i]->type == type) || (type == ANY)) )
			return i;
	}

	return -1;
}

/*
 * Picks an address given the scope of dst_addr 
 */
int esix_intf_pick_source_address(const esix_ip6_addr *dst_addr)
{
	int i;
	//try go get an address of the same scope. Note that we assume
	//that a link local address is always avaiable (SLAAC link local)
	if((dst_addr->raw[0] & hton32(0xffff0000)) == hton32(0xfe800000))
		i = esix_intf_get_type_address(LINK_LOCAL);
	else
		i = esix_intf_get_type_address(GLOBAL);

	return i;
}

/*
 * Return the address row index of the given address.
 */
int esix_intf_get_address_index(const esix_ip6_addr *addr, enum type type, uint8_t masklen)
{
	int j;
	for(j = 0; j<ESIX_MAX_IPADDR; j++)
	{
		//check if we already stored this address
		if((addrs[j] != NULL) &&
			((addrs[j]->type == type) || (type == ANY)) &&
			(addrs[j]->addr.raw[0] == addr->raw[0]) &&
			(addrs[j]->addr.raw[1] == addr->raw[1]) &&
			(addrs[j]->addr.raw[2] == addr->raw[2]) &&
			(addrs[j]->addr.raw[3] == addr->raw[3]) &&
			((addrs[j]->mask == masklen) || (masklen == 0xff)))

			return j;
	}
	return -1;
}

/*
 * Return the route row index of the given route.
 */
int esix_intf_get_route_index(const esix_ip6_addr *dst_addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop, const uint8_t intf)
{
	int i;
	for(i = 0; i<ESIX_MAX_RT; i++)
	{
		//check if we already stored this address
		if((routes[i] != NULL) &&
			(routes[i]->addr.raw[0]		== dst_addr->raw[0]) &&
			(routes[i]->addr.raw[1]		== dst_addr->raw[1]) &&
			(routes[i]->addr.raw[2]		== dst_addr->raw[2]) &&
			(routes[i]->addr.raw[3]		== dst_addr->raw[3]) &&
			(routes[i]->next_hop.raw[0]	== next_hop->raw[0]) &&
			(routes[i]->next_hop.raw[1]	== next_hop->raw[1]) &&
			(routes[i]->next_hop.raw[2]	== next_hop->raw[2]) &&
			(routes[i]->next_hop.raw[3]	== next_hop->raw[3]) &&
			(routes[i]->mask.raw[0]		== mask->raw[0])     &&
			(routes[i]->mask.raw[1]		== mask->raw[1])     &&
			(routes[i]->mask.raw[2]		== mask->raw[2])     &&
			(routes[i]->mask.raw[3]		== mask->raw[3])     &&
			(routes[i]->interface		== intf))
		
			return i;
	}
	return -1;
}

/**
 * esix_new_addr : creates an addres with the passed arguments
 * and adds or updates it.
 */
int esix_intf_add_address(esix_ip6_addr *addr, uint8_t masklen, uint32_t expiration_date, enum type type)
{
	struct esix_ipaddr_table_row *row;
	esix_ip6_addr	mcast_sollicited, zero;
	int i, j;

	i = esix_intf_get_address_index(addr, type, masklen);
	if(i >= 0)
	{
		//if we already have it and if it's supposed to expire
		//just update the expiration date and we're done.
		if(addrs[i]->expiration_date != 0)
			addrs[i]->expiration_date = expiration_date;

		return 1;
	}

	//we're still here, let's create our new address.
	row = malloc(sizeof(struct esix_ipaddr_table_row)); 

	//hmmmm... I smell gas...
	if(row == NULL)
		return 0;

	row->addr.raw[0]		= addr->raw[0];
	row->addr.raw[1]		= addr->raw[1];
	row->addr.raw[2]		= addr->raw[2];
	row->addr.raw[3]		= addr->raw[3];
	row->expiration_date	= expiration_date;
	row->type		= type;
	row->mask		= masklen;

	
	//it's new. if it's unicast, perform DAD.
	//if it's anycast, don't perform DAD as duplicates are expected.
	//if it's multicast, don't do anything special.

	//perform DAD
	if(type == LINK_LOCAL || type == GLOBAL)	
	{
		zero.raw[0] = 0;
		zero.raw[1] = 0;
		zero.raw[2] = 0;
		zero.raw[3] = 0;

		for(i=0; i< DUP_ADDR_DETECT_TRANSMITS; i++)
		{
			esix_icmp6_send_neighbor_sol(&zero, addr);

			//TODO : hell, we need a proper sleep()...
			for(j=0; j<10000; j++)
				asm("nop");
		}

		//if we received an answer, bail out.
		if(esix_intf_get_neighbor_index(addr, INTERFACE) >= 0)
		{
			//memory leaks are evil
			free(row);
			return 0;
		}
	}//DAD succeeded, we can go on.


	//try to add it to the table of addresses and
	//free it so we don't leak memory in case it fails
	if(esix_intf_add_address_row(row)) 
	{
		if(type != MULTICAST)
		//try to add the multicast solicited-node address (for neighbor discovery)
		{
			mcast_sollicited.raw[0] = 	hton32(0xff020000); 	//0xff02 
			mcast_sollicited.raw[1] = 	0;
			mcast_sollicited.raw[2] = 	hton32(1);
			mcast_sollicited.raw[3] = 	hton32(	(0xff000000)
							| (ntoh32(row->addr.raw[3])  & 0x00ffffff));
			
			//not having this address breaks the ND protocol, so don't add the 
			//unicast/anycast address if this fails
			esix_intf_add_address(&mcast_sollicited,
						0x80,			// /128
					expiration_date, 	//expires with the main address
					MULTICAST);

		}
		//else
		//	esix_icmp_send_mld(&row->addr, MLD_RPT);

		return 1;
	}

	//if we're still here, something went wrong.
	esix_intf_remove_address(&mcast_sollicited, 0x80, MULTICAST);
	free(row);
	return 0;
}

int esix_intf_remove_address(const esix_ip6_addr *addr, enum type type, uint8_t masklen)
{
	int i;
	struct esix_ipaddr_table_row *row;
	//TODO : remove multicast sollicited-node address
	//only if no other unicast address uses it
	
	i = esix_intf_get_address_index(addr, type, masklen);
	if(i >= 0)
	{
              //send a MLD done report if this is a mcast address
                //if(type == MULTICAST)
                //        esix_icmp_send_mld(&row->addr, MLD_DNE);

		row = addrs[i];
		addrs[i] = NULL; 
		free(row);

		return 1;
	}
	return 0;
}

int esix_intf_add_route(const esix_ip6_addr *dst_addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_addr, uint32_t expiration_date,
				uint8_t ttl, uint32_t mtu, uint8_t interface)
{		
	int i=0;
	struct esix_route_table_row *rt;

	//try to look up this route in the table
	//to find if it already exists and only needs an update
	if( (i = esix_intf_get_route_index(dst_addr, mask, next_addr, interface)) >=0)
	{
		//we found something, just update some variables
		rt			= routes[i];
		if(rt->expiration_date != 0)
			rt->expiration_date	= expiration_date;
		rt->ttl			= ttl;
		rt->mtu			= mtu;
		return 1;
	}

	//we're still here, create the new route.
	rt	= malloc(sizeof(struct esix_route_table_row));

	//hmmmm... I smell gas...
	if(rt == NULL)
		return 0;

	rt->addr.raw[0]		= dst_addr->raw[0];
	rt->addr.raw[1]		= dst_addr->raw[1];
	rt->addr.raw[2]		= dst_addr->raw[2];
	rt->addr.raw[3]		= dst_addr->raw[3];
	rt->mask.raw[0]		= mask->raw[0];
	rt->mask.raw[1]		= mask->raw[1];
	rt->mask.raw[2]		= mask->raw[2];
	rt->mask.raw[3]		= mask->raw[3];
	rt->next_hop.raw[0]	= next_addr->raw[0];
	rt->next_hop.raw[1]	= next_addr->raw[1];
	rt->next_hop.raw[2]	= next_addr->raw[2];
	rt->next_hop.raw[3]	= next_addr->raw[3];
	rt->expiration_date	= expiration_date;
	rt->ttl			= ttl;
	rt->mtu			= mtu;
	rt->interface		= interface;

	//now try to add it
	if(esix_intf_add_route_row(rt))
		return 1;

	//we're still here, clean our mess up.
	free(rt);
	return 0;
} 

/*
 * esix_intf_remove_route : removes a route from the routing table.
 */
int esix_intf_remove_route(const esix_ip6_addr *dst_addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop, uint8_t intf)
{
	int i;
	struct esix_route_table_row *rt;
	if( (i = esix_intf_get_route_index(dst_addr, mask, next_hop, intf)) >= 0)
	{
		rt	= routes[i];
		routes[i] = NULL;
		free(rt);
		return 1;
	}
	return -1;
}

/*
 * esix_intf_check_source_addr : make sure that the source address isn't multicast
 * if it is, choose an address from the corresponding scope
 */
int esix_intf_check_source_addr(esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr)
{
	int i = -1;

	if(	(src_addr->raw[0] & hton32(0xff000000)) == hton32(0xff000000))
	{
		//try to chose an address of the correct scope to replace it.
		if((dst_addr->raw[0] & hton32(0xffff0000)) == hton32(0xfe800000))
			i = esix_intf_get_type_address(LINK_LOCAL);
		
		if( (i < 0 ) && (i = esix_intf_get_type_address(GLOBAL)) < 0)
			return -1;
							
		*src_addr	= addrs[i]->addr; 
	}
	return 1;
}

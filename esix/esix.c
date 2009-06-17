/**
 * @file
 * esix ipv6 stack main file.
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

#include "config.h"
#include "include/esix.h"

//general purpose buffer, mainly used to build packets/
//static int esix_buffer[ESIX_BUFFER_SIZE];

//this table contains every ip address assigned to the system.
struct 	esix_ipaddr_table_row *addrs[ESIX_MAX_IPADDR];

//this table contains every routes assigned to the system
struct 	esix_route_table_row *routes[ESIX_MAX_RT];

//stores our mac addr, for now...
u16_t mac_addr[3];

/**
 * esix_init : sets up the esix stack.
 */
void esix_init(void)
{
	int i;
	for(i=0; i<ESIX_MAX_IPADDR; i++)
		addrs[i] = NULL;

	for(i=0; i<ESIX_MAX_RT; i++)
		routes[i] = NULL;


	//change here to get a dest mac address out of your ethernet
	//driver if needed. Implementing ether_get_mac_addr() in your driver
	//is the best way.
	//output must be in network order (big endian)

	ether_get_mac_addr(mac_addr);

	add_basic_addr_routes(mac_addr, INTERFACE, 1500);
	//esix_send_router_sollicitation(INTERFACE);
}


/**
 * hton16 : converts host endianess to big endian (network order) 
 */
u16_t hton16(u16_t v)
{
	if(ENDIANESS)
		return v;

	u8_t * tmp	= (u8_t *) &v;
	return (tmp[0] << 8 | tmp[1]);
}

/**
 * hton32 : converts host endianess to big endian (network order) 
 */
u32_t hton32(u32_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 24) & 0xff000000) |
	       ((v << 8) & 0x00ff0000) |
	       ((v >> 8) & 0x0000ff00) |
	       ((v >> 24) & 0x000000ff);
}

/**
 * ntoh16 : converts network order to host endianess
 */
u16_t ntoh16(u16_t v)
{
	if(ENDIANESS)
		return v;

	//bug due to alignment weirdiness
	//u8_t * tmp	= (u8_t *) &v;
	//return (tmp[1] << 8 | tmp[0]);
	return (((v << 8) & 0xff00) | ((v >> 8) & 0x00ff));
}

/**
 * ntoh32 : converts network order to host endianess 
 */
u32_t ntoh32(u32_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 24) & 0xff000000) |
	       ((v << 8) & 0x00ff0000) |
	       ((v >> 8) & 0x0000ff00) |
	       ((v >> 24) & 0x000000ff);
}

/**
 * add_basic_addr_routes : adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void add_basic_addr_routes(u16_t *mac_addr, int intf_index, int intf_mtu)
{
	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.

	//unicast link local
	struct esix_ipaddr_table_row *ucast_ll = MALLOC (sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	ucast_ll->addr.addr1	= hton32(0xfe800000); // fe80/8 is the link local unicast prefix 
	ucast_ll->addr.addr2	= hton32(0x00000000);

	//last 64 bits
	u8_t	*tmp	= (u8_t*) &mac_addr[1] ; //split a half word in two bytes so we can use them
						//separately
	ucast_ll->addr.addr3	= hton32(
					mac_addr[0] << 16 | tmp[1] << 8 | 0xff);
	ucast_ll->addr.addr4	= hton32(0xfe << 24 | tmp[0] << 16 | mac_addr[2]);

	//this one never expires
	ucast_ll->expiration_date	= 0x0;
	ucast_ll->scope			= LINK_LOCAL_SCOPE;

	//multicast link local associated address (for neighbor discovery)
	struct esix_ipaddr_table_row *mcast_ll = MALLOC (sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	mcast_ll->addr.addr1	= hton32(0xff020000);
	mcast_ll->addr.addr2	= hton32(0x00000000);

	//last 64 bits
	mcast_ll->addr.addr3	= hton32(0x00000001);

	//TODO: endianess support & testing 
	//struct ether_addr with some unions in it avoiding endianess weirdiness?
	/*if(ENDIANESS)
		mcast_ll->addr.addr4	= hton32(0xff << 24 | tmp[1] << 16 | mac_addr[2]);
	else*/
	mcast_ll->addr.addr4	= hton32(0xff << 24 | tmp[0] << 16 | mac_addr[2]);

	//this one never expires
	mcast_ll->expiration_date	= 0x0;
	mcast_ll->scope			= MCAST_SCOPE;

	//multicast all-nodes (for router advertisements)
	
	struct esix_ipaddr_table_row *mcast_all = MALLOC (sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	mcast_all->addr.addr1	= hton32(0xff020000);
	mcast_all->addr.addr2	= hton32(0x00000000);

	mcast_all->addr.addr3	= hton32(0x00000000);
	mcast_all->addr.addr4	= hton32(0x00000001);

	//this one never expires
	mcast_all->expiration_date	= 0x0;
	mcast_all->scope		= MCAST_SCOPE;

	//TODO perform DAD
	
	//add them to the table of active addresses
	esix_add_to_active_addresses(ucast_ll);
	esix_add_to_active_addresses(mcast_ll);
	esix_add_to_active_addresses(mcast_all);

	//link local route (fe80::/64)
	struct esix_route_table_row *ll_rt	= MALLOC(sizeof(struct esix_route_table_row));
	
	ll_rt->addr.addr1	= hton32(0xfe800000);
	ll_rt->addr.addr2	= 0x0;
	ll_rt->addr.addr3	= 0x0;
	ll_rt->addr.addr4	= 0x0;
	ll_rt->mask		= 64;
	ll_rt->next_hop.addr1	= 0x0; //a value of 0 means no next hop 
	ll_rt->next_hop.addr1	= 0x0; 
	ll_rt->next_hop.addr1	= 0x0; 
	ll_rt->next_hop.addr1	= 0x0; 
	ll_rt->ttl		= DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	ll_rt->mtu		= intf_mtu;	
	ll_rt->expiration_date	= 0x0; //this never expires
	ll_rt->interface	= intf_index;

	//multicast route (ff00:: /8)
	struct esix_route_table_row *mcast_rt	= MALLOC(sizeof(struct esix_route_table_row));
	
	mcast_rt->addr.addr1		= hton32(0xff000000);
	mcast_rt->addr.addr2		= 0x0;
	mcast_rt->addr.addr3		= 0x0;
	mcast_rt->addr.addr4		= 0x0;
	mcast_rt->mask			= 8;
	mcast_rt->next_hop.addr1	= 0x0; //a value of 0 means no next hop 
	mcast_rt->next_hop.addr1	= 0x0; 
	mcast_rt->next_hop.addr1	= 0x0; 
	mcast_rt->next_hop.addr1	= 0x0; 
	mcast_rt->expiration_date	= 0x0; //this never expires
	mcast_rt->ttl			= DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	mcast_rt->mtu			= intf_mtu;
	mcast_rt->interface		= intf_index;

	esix_add_to_active_routes(ll_rt);
	esix_add_to_active_routes(mcast_rt);
}


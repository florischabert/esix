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

#include "nd6.h"
#include "tools.h"
#include "ip6.h"
#include "icmp6.h"
#include "eth.h"
#include <esix.h>

static esix_list nd6_addrs;
static esix_list nd6_routes;
static esix_list nd6_neighbors;

static esix_eth_addr nd6_lla = {{ 0, 0, 0 }};

esix_eth_addr esix_nd6_lla(void)
{
	return nd6_lla;
}

static esix_ip6_addr nd6_create_link_local_addr(esix_eth_addr eth_addr)
{
	esix_ip6_addr ip_addr = {{
		0xfe800000,
		0x00000000,
		((eth_addr.raw[0] << 16) & 0xff0000)
			| (eth_addr.raw[1] & 0xff00)
			| 0x020000ff,
		0xfe000000
			| ((eth_addr.raw[1] << 16) & 0xff0000)
			| eth_addr.raw[2]
	}};
	return ip_addr;
}

static esix_ip6_addr nd6_create_multicast_all_nodes_addr(void)
{
	esix_ip6_addr ip_addr = {{
		0xff020000, 0, 0, 1
	}};
	return ip_addr;
}

esix_err esix_nd6_init(esix_lla lla)
{
	esix_err err = esix_err_none;

	esix_list_init(&nd6_addrs);
	esix_list_init(&nd6_routes);
	esix_list_init(&nd6_neighbors);

	nd6_lla.raw[0] = (lla[0] << 8) | lla[1];
	nd6_lla.raw[1] = (lla[2] << 8) | lla[3];
	nd6_lla.raw[2] = (lla[4] << 8) | lla[5];

	esix_nd6_autoconfigure(nd6_lla);

	return err;
}

esix_err esix_nd6_add_default_routes(int nd6_mtu)
{
	esix_err err = esix_err_none;
	esix_nd6_route *route;

	// link local route (fe80::/64)
	route = malloc(sizeof(*route));
	if (!route) {
		err = esix_err_oom;
		goto out;
	}
	
	route->addr = esix_ip6_addr_create(0xfe800000, 0x0, 0x0, 0x0);
	route->mask = esix_ip6_addr_create(0xffffffff, 0xffffffff, 0x0, 0x0);
	route->next_hop = esix_ip6_addr_create(0x0, 0x0, 0x0, 0x0); //TODO change next_hop == 0 destination
	route->ttl = DEFAULT_TTL; 
	route->mtu = nd6_mtu;
	route->expiration_date = 0x0; //this never expires

	esix_list_add(&route->link, &nd6_routes);

	// multicast route (ff00:: /8)
	route = malloc(sizeof(*route));
	if (!route) {
		err = esix_err_oom;
		goto out;
	}
	
	route->addr = esix_ip6_addr_create(0xff000000, 0x0, 0x0, 0x0);
	route->mask = esix_ip6_addr_create(0xff000000, 0x0, 0x0, 0x0);
	route->next_hop = esix_ip6_addr_create(0x0, 0x0, 0x0, 0x0);
	route->ttl = DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	route->mtu = nd6_mtu;

	esix_list_add(&route->link, &nd6_routes);
out:
	return err;
}

esix_err esix_nd6_autoconfigure(esix_eth_addr eth_addr)
{
	esix_ip6_addr ip_addr;
	esix_err err = esix_err_none;

	nd6_lla = eth_addr;

	ip_addr = nd6_create_link_local_addr(eth_addr);
	err = esix_nd6_add_addr(&ip_addr, 128, 0, esix_ip6_addr_type_link_local);
	if (err) {
		goto out;
	}
	err = esix_nd6_add_neighbor(&ip_addr, &eth_addr, 0);
	if (err) {
		goto out;
	}

	ip_addr = nd6_create_multicast_all_nodes_addr();
	err = esix_nd6_add_addr(&ip_addr, 128, 0, esix_ip6_addr_type_multicast);

	esix_nd6_add_default_routes(1500);

out:
	return err;
}

esix_err esix_nd6_add_neighbor(const esix_ip6_addr *ip_addr, const esix_eth_addr *eth_addr, uint32_t expiration_date)
{
	esix_err err = esix_err_none;
	esix_nd6_neighbor *neighbor;

	neighbor = esix_nd6_get_neighbor(ip_addr);
	if (!neighbor) {
		neighbor = malloc(sizeof(*neighbor));
		if (!neighbor) {
			err = esix_err_oom;
			goto out;
		}
		neighbor->addr = *ip_addr;
		esix_list_add(&neighbor->link, &nd6_neighbors);
	}

	neighbor->expiration_date = expiration_date;
	neighbor->lla = *eth_addr;
out:
	return err;
}

esix_nd6_neighbor *esix_nd6_get_neighbor(const esix_ip6_addr *addr)
{
	esix_nd6_neighbor *neighbor;

	esix_list_foreach(neighbor, &nd6_neighbors, link) {
		if (esix_ip6_addr_match(&neighbor->addr, addr)) {
			goto out;
		}
	}
	neighbor = NULL;
out:
	return neighbor;
}

void esix_nd6_remove_neighbor(const esix_ip6_addr *addr)
{
	esix_nd6_neighbor *neighbor;
	
	neighbor = esix_nd6_get_neighbor(addr);
	if (neighbor) {
		esix_list_del(&neighbor->link);
		free(neighbor);
	}
}

esix_nd6_addr *esix_nd6_get_addr_for_type(esix_ip6_addr_type type)
{
	esix_nd6_addr *addr;

	esix_list_foreach(addr, &nd6_addrs, link) {
		if (addr->type == type) {
			goto out;
		}
	}
	addr = NULL;
out:
	return addr;
}

static int nd6_addr_is_link_local(const esix_ip6_addr *addr)
{
	return (addr->raw[0] & 0xffff0000) == 0xfe800000;
}

esix_nd6_addr *esix_nd6_pick_src_addr(const esix_ip6_addr *dst_addr)
{
	esix_ip6_addr_type type = esix_ip6_addr_type_global;

	if (nd6_addr_is_link_local(dst_addr)) {
		type = esix_ip6_addr_type_link_local;
	}

	return esix_nd6_get_addr_for_type(type);
}

esix_nd6_addr *esix_nd6_get_addr(const esix_ip6_addr *dst_addr, esix_ip6_addr_type type, uint8_t masklen)
{
	esix_nd6_addr *nd6_addr;

	esix_list_foreach(nd6_addr, &nd6_addrs, link) {
		if (esix_ip6_addr_match(&nd6_addr->addr, dst_addr) &&
			(nd6_addr->type == type || type == esix_ip6_addr_type_any) &&
			(nd6_addr->masklen == masklen || masklen == 0xff)) {
			goto out;
		}
	}
	nd6_addr = NULL;
out:
	return nd6_addr;
}

esix_nd6_route *esix_nd6_get_route(const esix_ip6_addr *dst_addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop)
{
	esix_nd6_route *route;

	esix_list_foreach(route, &nd6_routes, link) {
		if (esix_ip6_addr_match(&route->addr, dst_addr) &&
			esix_ip6_addr_match(&route->mask, mask) &&
			esix_ip6_addr_match(&route->next_hop, next_hop)) {
			goto out;
		}
	}
	route = NULL;
out:
	return route;
}

esix_err esix_nd6_add_addr(const esix_ip6_addr *addr, uint8_t masklen, uint32_t expiration_date, esix_ip6_addr_type type)
{
	esix_err err = esix_err_none;
	esix_nd6_addr *nd6_addr;
	int i, j;

	nd6_addr = esix_nd6_get_addr(addr, type, masklen);
	if (nd6_addr) {
		if (nd6_addr->expiration_date == 0) {
			nd6_addr->expiration_date = expiration_date;
		}
		goto out;
	}

	nd6_addr = malloc(sizeof(*nd6_addr));
	if (!nd6_addr) {
		err = esix_err_oom;
		goto out;
	}
	
	nd6_addr->addr = *addr;
	nd6_addr->type = type;
	nd6_addr->masklen = masklen;
	nd6_addr->expiration_date = expiration_date;
	
	//it's new. if it's unicast, perform DAD.
	//if it's anycast, don't perform DAD as duplicates are expected.
	//if it's multicast, don't do anything special.

	//perform DAD
	if (type == esix_ip6_addr_type_link_local || type == esix_ip6_addr_type_global) {
		esix_ip6_addr zero = {{ 0, 0, 0, 0 }};

		for (i = 0; i < DUP_ADDR_DETECT_TRANSMITS; i++) {
			esix_icmp6_send_neighbor_sol(&zero, addr);

			//TODO : hell, we need a proper sleep()...
			for (j=0; j<10000; j++)
				asm("nop");
		}

		//if we received an answer, bail out.
		if (esix_nd6_get_neighbor(addr)){
			free (nd6_addr);
			err = esix_err_failed;
			goto out;
		}
	}//DAD succeeded, we can go on.


	//try to add it to the table of addresses and
	//free it so we don't leak memory in case it fails
	esix_list_add(&nd6_addr->link, &nd6_addrs);

	if (type != esix_ip6_addr_type_multicast) {
		esix_ip6_addr mcast_sollicited;

		//try to add the multicast solicited-node address (for neighbor discovery)
		mcast_sollicited = esix_ip6_addr_create(
			0xff020000,
			0,
			1,
			0xff000000 | (nd6_addr->addr.raw[3]  & 0xffffff00)
		);
		
		//not having this address breaks the ND protocol, so don't add the 
		//unicast/anycast address if this fails
		esix_nd6_add_addr(&mcast_sollicited, 128, expiration_date, esix_ip6_addr_type_multicast);

	}
	//else
	//	esix_icmp_send_mld(&row->addr, MLD_RPT);
out:
	return err;
}

void esix_nd6_remove_addr(const esix_ip6_addr *addr, esix_ip6_addr_type type, uint8_t masklen)
{
	esix_nd6_addr *nd6_addr;
	//TODO : remove multicast sollicited-node address
	//only if no other unicast address uses it

	nd6_addr = esix_nd6_get_addr(addr, type, masklen);
	if (nd6_addr) {
		//send a MLD done report if this is a mcast address
		//if(type == MULTICAST)
		//	esix_icmp_send_mld(&row->addr, MLD_DNE);
		esix_list_del(&nd6_addr->link);
		free(nd6_addr);
	}
}

esix_err esix_nd6_add_route(const esix_ip6_addr *dst_addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_addr, uint32_t expiration_date, uint8_t ttl, uint32_t mtu)
{		
	esix_err err = esix_err_none;
	esix_nd6_route *route;

	route = esix_nd6_get_route(dst_addr, mask, next_addr);
	if (!route) {
		route = malloc(sizeof(*route));
		if (!route) {
			err = esix_err_oom;
			goto out;
		}
	}

	route->addr = *dst_addr;
	route->mask = *mask;
	route->next_hop = *next_addr;
	route->expiration_date = expiration_date;
	route->ttl = ttl;
	route->mtu = mtu;

	esix_list_add(&route->link, &nd6_routes);	
out:
	return err;
} 

void esix_nd6_remove_route(const esix_ip6_addr *dst_addr, const esix_ip6_addr *mask, const esix_ip6_addr *next_hop)
{
	esix_nd6_route *route;

	route = esix_nd6_get_route(dst_addr, mask, next_hop);
	if (route) {
		esix_list_del(&route->link);
		free(route);
	}
}

int esix_nd6_check_source_addr(esix_ip6_addr *src_addr, const esix_ip6_addr *dst_addr)
{
	esix_nd6_addr *nd6_addr = NULL;

	if ((src_addr->raw[0] & 0xff000000) == 0xff000000) {
		//try to chose an address of the correct scope to replace it.
		if ((dst_addr->raw[0] & 0xffff0000) == 0xfe800000) {
			nd6_addr = esix_nd6_get_addr_for_type(esix_ip6_addr_type_link_local);
		}

		if (!nd6_addr) {
			nd6_addr = esix_nd6_get_addr_for_type(esix_ip6_addr_type_global);
		}
		
		if (!nd6_addr) {
			return -1;
		}

		*src_addr = nd6_addr->addr;
	}
	return 1;
}

esix_nd6_route *esix_nd6_get_route_for_addr(const esix_ip6_addr *addr)
{
	esix_nd6_route *matched_route = NULL;
	esix_nd6_route *route;

	esix_list_foreach(route, &nd6_routes, link) {		
		if (esix_ip6_addr_match_with_mask(addr, &route->mask, &route->addr)) {
			matched_route = route;
			break;
		}
	}
	return route;
}

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
#include "icmp6.h"
#include "socket.h"
#include "include/esix.h"
#include "intf.h"
#include "eth.h"

static uint32_t	current_time;
void (*esix_send_callback)(void *data, int len);

static int eth_addr_from_str(esix_eth_addr addr, char *str)
{
	int i;

	if (esix_strlen(str) != esix_strlen("xx:xx:xx:xx:xx:xx")) {
		return -1;
	}

	for (i = 0; i < 6; i++, str += 3) {
		int j;
		uint8_t byte = 0;

		for (j = 0; j < 2; j++) {
			uint8_t val = (str[j] > '9')? str[j]-'a'+10 : str[j]-'0';
			byte +=  val << (4*(1-j));
		}

		addr[i] = byte;
	}
	return 0;
}

/**
 * Sets up the esix stack.
 */
void esix_init(char *lla, void (*send_callback)())
{
	int i;
	esix_eth_addr eth_addr;

	if (!send_callback) {
		return;
	}
	esix_send_callback = send_callback;

	if (eth_addr_from_str(eth_addr, lla) == -1) {
		return;
	}

	current_time = 1;	// 0 means "infinite lifetime" in our caches
	
	for(i=0; i<ESIX_MAX_IPADDR; i++)
		addrs[i] = NULL;

	for(i=0; i<ESIX_MAX_RT; i++)
		routes[i] = NULL;

	for(i=0; i<ESIX_MAX_NB; i++)
		neighbors[i] = NULL;

	esix_socket_init();
	
	esix_intf_add_default_routes(INTERFACE, 1500);
	esix_intf_init_interface(eth_addr, INTERFACE);

	esix_icmp_send_router_sol(INTERFACE);
}

uint32_t esix_get_time()
{
	return current_time;
}

/*
 * esix_ip_housekeep : takes care of cache entries expiration at the ip level
 */
static void esix_ip_housekeep()
{
	int i,j;
	//loop through the routing table
	for(i=0; i<ESIX_MAX_RT; i++)
	{
		if(routes[i] != NULL)
		{
			if(routes[i]->expiration_date != 0 && 
			   routes[i]->expiration_date < current_time)
				esix_intf_remove_route(&routes[i]->addr, &routes[i]->mask,
				&routes[i]->next_hop, INTERFACE);
		}
	}

	//loop through the address table
	for(i=0; i<ESIX_MAX_IPADDR; i++)
	{
		if(addrs[i] != NULL)
			if(addrs[i]->expiration_date != 0 && 
			   addrs[i]->expiration_date < current_time)
				esix_intf_remove_address(&addrs[i]->addr, addrs[i]->type, addrs[i]->mask);
	}

	//loop through the neighbor table
	for(i=0; i<ESIX_MAX_NB; i++)
	{
		if(neighbors[i] != NULL && neighbors[i]->expiration_date != 0)
		{
			
			//don't delete the entry immediately. Put it in STALE state and
			//send an unicast neighbor advertisement to refresh it.
			if((neighbors[i]->expiration_date - STALE_DURATION) < current_time)
			{
				if((j=esix_intf_get_type_address(LINK_LOCAL)) >= 0)	
					esix_icmp_send_neighbor_sol(&addrs[j]->addr, &neighbors[i]->addr);
			}
	
			//if it hasn't been refreshed after STALE_DURATION seconds,
			//the neighbor might be gone. Remove the entry.
			if(neighbors[i]->expiration_date < current_time)
				esix_intf_remove_neighbor(&neighbors[i]->addr, INTERFACE);
			
		}
	}
	return;
}


/*
 * esix_periodic_callback : must be called every second by the OS.
 */
void esix_periodic_callback()
{
	current_time++;

	//call ip_housekeep every 2 ticks
	if(current_time%2)
		esix_ip_housekeep();

	//call socket_housekeep every tick
	esix_socket_housekeep();
}

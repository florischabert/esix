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
#include "esix.h"
#include "icmp6.h"
#include "socket.h"
#include "include/esix.h"
#include "intf.h"
#include "tools.h"

static u32_t	current_time;

/**
 * Sets up the esix stack.
 */
void esix_init(u16_t lla[3])
{
	//big fat wait loop...
	int i;
	for(i=0; i<40000000;i++)
		asm("nop");

	current_time = 1;	// 0 means "infinite lifetime" in our table/caches
	
	for(i=0; i<ESIX_MAX_IPADDR; i++)
		addrs[i] = NULL;

	for(i=0; i<ESIX_MAX_RT; i++)
		routes[i] = NULL;

	for(i=0; i<ESIX_MAX_NB; i++)
		neighbors[i] = NULL;
	
	for(i=0; i<ESIX_MAX_SOCK; i++)
		sockets[i] = NULL;

	esix_intf_add_default_routes(INTERFACE, 1500);
	esix_intf_init_interface(lla, INTERFACE);
	uart_printf("init interface done\n");
	esix_icmp_send_router_sol(INTERFACE);
}

u32_t esix_get_time()
{
	return current_time;
}

/*
 * esix_periodic_callback : must be called every second by the OS.
 */
void esix_periodic_callback()
{
	current_time++;
	esix_housekeep();
}


/*
 * esix_housekeep : takes care of cache entries expiration.
 */
void esix_housekeep()
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
				esix_intf_remove_address(&addrs[i]->addr, addrs[i]->scope, addrs[i]->mask);
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
				if((j=esix_intf_get_scope_address(INTERFACE)) >= 0)	
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

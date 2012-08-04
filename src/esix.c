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
#include "intf.h"
#include "eth.h"
#include "tools.h"
#include <esix.h>

static esix_callbacks callbacks;

static esix_queue inqueue;
static esix_queue outqueue;

void esix_internal_init(void)
{
	esix_queue_init(&inqueue);
	esix_queue_init(&outqueue);
}

/**
 * Sets up the esix stack.
 */
esix_err esix_init(esix_lla lla, esix_callbacks _callbacks)
{
	esix_err err = esix_err_none;

	esix_internal_init();

	if (!_callbacks.send ||
	    !_callbacks.gettimeofday ||
	    !_callbacks.cond_timedwait ||
	    !_callbacks.cond_signal) {

		err = esix_err_badparam;
		goto out;
	}
	callbacks = _callbacks;

	esix_intf_init(lla);

	esix_socket_init();

	esix_icmp6_send_router_sol(INTERFACE);

out:
	return err;
}

esix_err esix_inqueue_push(esix_buffer *buffer)
{
	return esix_queue_push(buffer, &inqueue);
}

esix_buffer *esix_inqueue_pop(void)
{
	return esix_queue_pop(&inqueue);
}

esix_err esix_outqueue_push(esix_buffer *buffer)
{
	return esix_queue_push(buffer, &outqueue);
}

esix_buffer *esix_outqueue_pop(void)
{
	return esix_queue_pop(&outqueue);
}

esix_err esix_workloop(void)
{
	esix_err err = esix_err_none;
	esix_buffer *buffer;

	while (1) {
		uint64_t wait_time = 0;

		// process input frames
		buffer = esix_inqueue_pop();
		if (buffer) {
			esix_eth_process(buffer->data, buffer->len);
		}

		// housekeeping please
		err = esix_ip6_work();
		if (err) {
			goto out;
		}

		// send output frames
		buffer = esix_outqueue_pop();
		if (buffer) {
			callbacks.send(buffer->data, buffer->len);
		}
		
		callbacks.cond_timedwait(wait_time);
	}

out:
	return err;
}

esix_err esix_enqueue(void *payload, int len)
{
	esix_err err = esix_err_none;
	esix_buffer *buffer;

	if (!payload || len < 0 || len > ESIX_MTU) {
		err = esix_err_badparam;
		goto out;
	}
	
	buffer = esix_buffer_alloc(len);
	if (!buffer) {
		err = esix_err_oom;
		goto out;
	}

	memcpy(buffer->data, payload, len);

	esix_inqueue_push(payload);

	callbacks.cond_signal();

out:
	return err;
}

uint64_t esix_time()
{
	return callbacks.gettimeofday();
}

/*
 * esix_ip_housekeep : takes care of cache entries expiration at the ip level
 */
#if 0
static void esix_intf_housekeep()
{
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
					esix_icmp6_send_neighbor_sol(&addrs[j]->addr, &neighbors[i]->addr);
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
#endif
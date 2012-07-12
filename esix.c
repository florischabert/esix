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
#include "tools.h"

#define ESIX_BUF_SIZE 1500

typedef struct {
	esix_list list;
	void *buffer;
} esix_buffer_link;

typedef struct {
	esix_list list;
	int lock;
} esix_buffer_queue;

static esix_buffer_queue esix_inqueue;
static esix_buffer_queue esix_outqueue;

static uint32_t	current_time;

void (*esix_send_callback)(void *data, int len) = NULL;

void *esix_buffer_alloc(void)
{
	return malloc(ESIX_BUF_SIZE);
}

void esix_buffer_free(void *buffer)
{
	free(buffer);
}

static esix_err esix_queue_push(void *buffer, esix_buffer_queue *queue)
{
	esix_err err = esix_err_none;
	esix_buffer_link *link;

	link = malloc(sizeof *link);
	if (!link) {
		err = esix_err_oom;
		goto out;
	}
	
	link->buffer = buffer;

	esix_lock(&queue->lock);
	esix_list_add(&link->list, &queue->queue);
	esix_unlock(&queue->lock);

out:
	return err;
}

static void *esix_queue_pop(esix_buffer_queue *queue)
{
	esix_buffer_link *link;
	void *buffer = NULL;

	if (esix_list_is_empty(&queue->list)) {
		goto out;
	}

	esix_list_tail(link, &queue->list);

	buffer = link->buffer;

	esix_lock(&queue->lock);
	esix_list_del(&link->list);
	esix_unlock(&queue->lock);

	free(link);

out:
	return buffer;
}

esix_err esix_inqueue_push(void *buffer)
{
	return esix_queue_push(buffer, &esix_inqueue);
}

void *esix_inqueue_pop(void)
{
	return esix_queue_pop(&esix_inqueue);
}

esix_err esix_outqueue_push(void *buffer)
{
	return esix_queue_push(buffer, &esix_inqueue);
}

void *esix_outqueue_pop(void)
{
	return esix_queue_pop(&esix_inqueue);
}

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
			uint8_t val;
			if (str[j] >= '0' && str[j] <= '9') {
				val = str[j] - '0';
			}
			else if (str[j] >= 'A' && str[j] <= 'F') {
				val = str[j] - 'A' + 10;
			}
			else if (str[j] >= 'a' && str[j] <= 'f') {
				val = str[j] - 'a' + 10;
			}
			else {
				return -1;
			}
			byte +=  val << (4*(1-j));
		}

		addr.raw[i] = byte;
	}
	return 0;
}

/**
 * Sets up the esix stack.
 */
esix_err esix_init(char *lla)
{
	int i;
	esix_eth_addr eth_addr;

	esix_list_init(&esix_inqueue);
	esix_list_init(&esix_outqueue);

	if (eth_addr_from_str(eth_addr, lla) == -1) {
		return;
	}

	current_time = 1;	// 0 means "infinite lifetime" in our caches

	esix_intf_init();
	esix_socket_init();

	esix_intf_autoconfigure(eth_addr);

	esix_icmp6_send_router_sol(INTERFACE);
}

esix_err esix_worker(void (*send_callback)())
{
	if (!send_callback) {
		return;
	}
	esix_send_callback = send_callback;
}

uint32_t esix_get_time()
{
	return current_time;
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
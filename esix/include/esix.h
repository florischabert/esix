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

#ifndef _ESIX_H
#define _ESIX_H

// Lib services

	/**
	 * Sets up the esix stack.
	 *
	 * @param lla is the 6 bytes link-layer address of the interface.
	 */
	void esix_init(u16_t lla[3]);
	
	/*
	 * Process a received IPv6 packet.
	 * 
	 * @param packet is a pointer to the packet.
	 * @param len is the packet size.
	 */
	void esix_ip_process(void *packet, int len);

	/*
	 * ipv6 stack clock signal.
	 *
	 * Needs to be called every second by the user.
	 *
	 */
	void esix_periodic_callback();
	
// The following has to be implemented by the user

	/*
	 * Malloc wrapper for esix.
	 * 
	 * Needs to be implemented by the user.
	 *
	 * @param size is the number of byte needed for the allocation.
	 */
	void *esix_w_malloc(size_t size);
	
	/*
	 * Free wrapper for esix.
	 * 
	 * Needs to be implemented by the user.
	 *
	 * @param is the a pointer to the zone to be freed.
	 */
	void esix_w_free(void *);

	/*
	 * Give the current time in ms.
	 *
	 * Needs to be implemented by the user.
	 *
	 * @return the time in ms (only for relative delays)
	 */
	u32_t esix_w_get_time(void);

	/*
	 * Send an IPv6 packet.
	 *
	 * Needs to be implemented by the user.
	 *
	 * @param lla is the target 6 bytes link-layer address.
	 * @param packet is a ptr to the IPV6 packet. should be freed after copy.
	 * @param len is the len of the IPv6 packet in bytes.
	 */
	void esix_w_send_packet(u16_t lla[3], void *packet, int len);
#endif

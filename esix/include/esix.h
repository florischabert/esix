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

	/**
	 * Sets up the esix stack.
	 */
	void esix_init(void);

	/*
	 * Malloc wrapper for esix.
	 * 
	 * Needs to be implemented by the user.
	 */
	void *esix_w_malloc(size_t size);
	
	/*
	 * Free wrapper for esix.
	 * 
	 * Needs to be implemented by the user.
	 */
	void esix_w_free(void *);

	/*
	 * Give the current time in ms.
	 *
	 * Needs to be implemented by the user.
	 */
	u32_t esix_w_get_time(void);
	
	/*
	 * Log print wrapper for esix.
	 * 
	 * Needs to be implemented by the user.
	 */
	void esix_w_log(char *string);
	
	/*
	 * MAC address (48 bits).
	 */
	struct esix_mac_addr
	{
		u16_t h;
		u32_t l;
	};
	
	/*
	 * Give the interface MAC address.
	 *
	 * Needs to be implemented by the user.
	 */
	struct esix_mac_addr esix_w_get_mac_address(void);
	
	/*
	 * Process a received IPv6 packet.
	 * 
	 * @param packet is a pointer to the packet.
	 * @parmr length is the packet size.
	 */
	void esix_ip_process_packet(void *packet, int length);
	
#endif

/**
 * @file
 * LM3S6965 ethernet driver.
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

#ifndef _ETHERNET_H
#define _ETHERNET_H
	#include "types.h"
	#include <FreeRTOS.h>
	#include <queue.h>

	#define MAX_FRAME_SIZE 380 //380 * 4 bytes = 1520 bytes
	
	void ether_init(u16_t lla[3]);
	void ether_enable(void);
	void ether_disable(void);
	void ether_handler(void);
	void ether_mii_transaction_complete(void);
	void ether_phy_int(void);
	void ether_fifo_overrun(void);
	void ether_tx_error(void);
	void ether_rx_error(void);
	void ether_txfifo_empty(void);
	void ether_frame_received(void);
	void ether_mii_request(u32_t, u32_t*, int);
	void ether_send(u16_t dlla1, u16_t dlla2, u16_t dlla3, u16_t type, void *data, int len);
	
	// Send queue (takes struct ether_frame_t)
	xQueueHandle ether_send_queue;
	
	/**
	 * Ethernet frame (FCS is hardware generated/appended) 
	 */
	struct ether_hdr_t {
		u16_t FRAME_LENGTH; //this field is added by the eth controller
		u16_t DA_1;
		u16_t DA_2;
		u16_t DA_3;
		u16_t SA_1;
		u16_t SA_2;
		u16_t SA_3;
		u16_t ETHERTYPE;
	} __attribute__((__packed__));
	
	struct ether_frame_t {
		struct ether_hdr_t hdr;
		u32_t *data;
	} __attribute__((__packed__));
	
	#define MII_READ  1
	#define MII_WRITE 0

	#define PHYINT 	(1 << 6)
	#define MDINT 	(1 << 5)
	#define RXER 	(1 << 4)
	#define FOV 	(1 << 3)
	#define TXEMP 	(1 << 2)
	#define TXER 	(1 << 1)
	#define RXINT 	(1 << 0)
	
#endif

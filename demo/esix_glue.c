/**
 * @file
 * Provide wrappers for esix.
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

#include <FreeRTOS.h>
#include "types.h"
#include <esix.h>
#include "mmap.h"
#include "uart.h"
#include "ethernet.h"

#define HTON16(v) (((v << 8) & 0xff00) | ((v >> 8) & 0x00ff))

void *esix_w_malloc(size_t size)
{
	return pvPortMalloc(size);
}

void esix_w_free(void *ptr)
{
	vPortFree(ptr);
}

u32_t esix_w_get_time(void)
{
	return 0;
}

void esix_w_send_packet(u16_t lla[3], void *packet, int len)
{
	int i;
	eth_f = pvPortMalloc(sizeof(struct ether_frame_t) + len);
	eth_f->FRAME_LENGTH = len;
	eth_f->DA_1 = HTON16(lla[0]);
	eth_f->DA_2 = HTON16(lla[1]);
	eth_f->DA_3 = HTON16(lla[2]);
	eth_f->SA_1 = HTON16(ETH0->MACIA0 >> 16);
	eth_f->SA_2 = HTON16(ETH0->MACIA0);
	eth_f->SA_3 = HTON16(ETH0->MACIA1);
	eth_f->ETHERTYPE = HTON16(0x86dd); // IPv6 packet
	for(i = 0; i < len; i++)
		*(((u32_t *) (eth_f + 1)) + i) =  *(((u32_t *) packet) + i);

	vPortFree(packet);
	ether_send_start();
}

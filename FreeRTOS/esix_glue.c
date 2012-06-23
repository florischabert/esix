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
#include "task.h"
#include "queue.h"
#include "types.h"
#include <esix.h>
#include "mmap.h"
#include "uart.h"
#include "ethernet.h"

int alloc_count=0;

void *malloc(size_t size)
{
	int i;
	u32_t *ptr;
 	ptr = pvPortMalloc(size);
	if(ptr == NULL)
	{
		uart_printf("ERROR: malloc failed (size=%x) count = %x\n", size,alloc_count);
		for(i = 0; i < 10000; i++) asm("nop");
	}
	else
		alloc_count++;
	return ptr;
}

void free(void *ptr)
{
	alloc_count--;
	vPortFree(ptr);
}

u32_t get_time(void)
{
	return 0;
}

void send_packet(u16_t lla[3], void *packet, int len)
{
	struct ether_frame_t eth_f;

	eth_f.hdr.FRAME_LENGTH = len;
	eth_f.hdr.DA_1 = (lla[0]);
	eth_f.hdr.DA_2 = (lla[1]);
	eth_f.hdr.DA_3 = (lla[2]);
	eth_f.hdr.SA_1 = ETH0->MACIA0;
	eth_f.hdr.SA_2 = (ETH0->MACIA0 >> 16);
	eth_f.hdr.SA_3 = ETH0->MACIA1;
	eth_f.hdr.ETHERTYPE = HTON16(0x86dd);
	eth_f.data = packet;

	xQueueSend(ether_send_queue, &eth_f, portMAX_DELAY);	
}

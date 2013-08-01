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

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "ethernet.h"
#include "mmap.h"
#include "uart.h"
#include <esix.h>

// Mutexes
static xSemaphoreHandle ether_receive_sem;

// Prototypes
void ether_receive_task(void *param);
void ether_send_task(void *param);

/**
 * ether_init : configures the ethernet hardware found in lm3s6965 
 */
void ether_init(u16_t lla[3])
{
	SYSCTR->RCGC2	|= (1 << 30); //enable PHY clock 
	SYSCTR->RCGC2	|= (1 << 28); //enable MAC clock

	//give them a little time to wake up
	asm("nop");

	//set the MAC Management frequency (must be <2.5MHz,
	//Fmfq = Fcpu/(2*(MACDVR+1)) )
	//still dunno at what speed we're running. 50MHz now?
	ETH0->MACMDV	= 0x00000009;

/*
	003a e967 c58d;
*/
	//program a MAC address 
	ETH0->MACIA0 = HTON32(0x003ae967);	//4 first MAC address bytes
	ETH0->MACIA1 = HTON16(lla[2]); //the first 2 bytes are marked as reserved

	//generate FCS in hardware, do autoneg, FD.
	ETH0->MACTCTL	|= 0x00000016;

	//flush rx fifo, reject frames with bad CRC in hardware
	ETH0->MACRCTL	|= 0x00000018;

	//tell the hardware to hand over multicast frames. We're going to need 
	//them for IPv6 and this ether chip doesn't have any mcast filter list.
	ETH0->MACRCTL	|= 0x00000002;

	//set leds to be controlled by hardware
	GPIOF->AFSEL |= 0x0000000c;
	GPIOF->ODR   |= 0x0000000c;
	
	ETH0->MACIM	&= ~0x0000007f;

	//Unmask ethernet interrupt at the processor level and set
	//a priority of configMAX_SYSCALL_INTERRUPT_PRIORITY+2
	NVIC->IPR[10] &= ~(0xff << 16); // Set interrupt priority
	NVIC->IPR[10] |= ((configMAX_SYSCALL_INTERRUPT_PRIORITY+2) << 16);
	NVIC->ISER[1] |= (1 << 10); // Unmask ethernet interrupt

	vSemaphoreCreateBinary(ether_receive_sem);
	xSemaphoreTake(ether_receive_sem, portMAX_DELAY);
	ether_send_queue = xQueueCreate(3, sizeof(struct ether_frame_t));
	xTaskCreate(ether_receive_task, (signed char *) "eth receive", 200, NULL, tskIDLE_PRIORITY + 4, NULL);
	xTaskCreate(ether_send_task, (signed char *) "eth send", 200, NULL, tskIDLE_PRIORITY + 3, NULL);
}

/**
 * ether_enable : activates the ethernet interface
 */
void ether_enable()
{

	//TX and RX are started independantly
	//RX
	ETH0->MACRCTL	|= 0x00000001;
	//TX
	ETH0->MACTCTL	|= 0x00000001;
	//Unmask all ethernet interupts
	//at the controller level
	ETH0->MACIM	|= 0x0000001;

	//Unmask interrupts at the MII level

/*
	u32_t mii_val = 0xffff;
	ether_mii_request(0x11, &mii_val, MII_WRITE);
        ETH0->MACMTXD |= 0xff00;
	
	u16_t addr = 0x11;
        u16_t tmp     = (addr << 3 ) + 0x03; //0x3 = write + start
        ETH0->MACMCTL = tmp;
*/
}

/**
 * ether_disable : disables the ethernet interface
 */
void ether_disable()
{
	//TX and RX are started independantly
	//RX
	ETH0->MACRCTL	&= ~0x00000001;
	//TX
	ETH0->MACTCTL	&= ~0x00000001;

	//mask interrupts
	ETH0->MACIM	&= ~0x0000007f;
}

/**
 * ether_handler : Ethernet interrupt handler
 */
void ether_handler()
{
	u16_t int_val	= ETH0->MACRIS;
	static portBASE_TYPE resched_needed; 
	resched_needed = pdFALSE;

	if(int_val&RXINT)
	{
		xSemaphoreGiveFromISR(ether_receive_sem, &resched_needed);
		ETH0->MACIM	&= ~0x1;
	}
	if(int_val&TXEMP)
		ether_txfifo_empty();
	if(int_val&RXER)
		ether_rx_error();
	if(int_val&TXER)
		ether_tx_error();
	if(int_val&FOV)
		ether_fifo_overrun();
	if(int_val&PHYINT)
		ether_phy_int();
	if(int_val&MDINT)
		ether_mii_transaction_complete();
		
	portEND_SWITCHING_ISR(resched_needed);	
}

/**
 * ether_frame_received : called after an interrupt has been received.
 * copies a frame from the RX ring buffer to a loacl buffer.
 * 
 */
void ether_receive_task(void *param)
{
	int i;
	int len;
	static char buf[MAX_FRAME_SIZE];
	u32_t *eth_buf = (u32_t *) buf;
	struct ether_hdr_t hdr;
	
	while(1)
	{
		xSemaphoreTake(ether_receive_sem, portMAX_DELAY);

		// read the header (16 bytes)
		for(i = 0; i < 4; i++)
			*((u32_t*) &hdr + i) = ETH0->MACDATA;
		
		len =	hdr.FRAME_LENGTH - 20;
		
		// we process the packet only if it's not too big and if it contains IPv6
		if((len > MAX_FRAME_SIZE - 20) || (hdr.ETHERTYPE != 0xdd86))
		{
			i=0;
			while(i++ < len)
				(u32_t) ETH0->MACDATA;
		}
		else
		{	
			// read the payload
			for(i = 0; i < len; i += 4)
				*(eth_buf+4+i/4) = ETH0->MACDATA;
				
			//got a v6 frame, pass it to the v6 stack
			esix_ip_process((eth_buf + 4), len);
		}
		
		// read checksum
		(u32_t) ETH0->MACDATA;
		
		// is the RX FIFO empty ?
		if(ETH0->MACRIS & RXINT)
			xSemaphoreGive(ether_receive_sem);
		else
			ETH0->MACIM	|= 0x1;
	}
}

/*
 * Send an ethernet frame.
 */

void ether_send_task(void *param)
{
	int i;
	int len4;
	struct ether_frame_t eth_f;
	
	while(1)	
	{
		xQueueReceive(ether_send_queue, &eth_f, portMAX_DELAY);

		// get the packet length
		len4 = eth_f.hdr.FRAME_LENGTH/4;
		if(eth_f.hdr.FRAME_LENGTH % 4)
			len4++;

		//send the header
		for(i = 0; i < 4; i++)
			ETH0->MACDATA = *(((u32_t *) &eth_f.hdr) + i);			

		//now send the data
		for(i = 0; (i < len4) && (i < MAX_FRAME_SIZE-5); i++) 
				ETH0->MACDATA = *(eth_f.data + i);
				
		esix_free(eth_f.data);
			
		ETH0->MACTR |= 1; // now, start the transmission
		while(ETH0->MACTR & 0x1); // waiting for the transmission to be complete	
	}
}

void ether_txfifo_empty()
{
	
//	GPIOF->DATA[1] ^= 1;
}

void ether_rx_error()
{
	
//	GPIOF->DATA[1] ^= 1;
}

void ether_tx_error()
{
//	GPIOF->DATA[1] ^= 1;
	
}

void ether_fifo_overrun()
{
	
//	GPIOF->DATA[1] ^= 1;

}

void ether_phy_int()
{
	
//	GPIOF->DATA[1] ^= 1;
}

void ether_mii_transaction_complete()
{
	
//	GPIOF->DATA[1] ^= 1;
}

/**
 * Helper function to get/sec MII registers.
 *
 * When write == 0, the content of val is written to the specified MII register.
 * When write != 0, the specified MII register is read and its value is written to
 * val.
 */
void ether_mii_request(u32_t addr, u32_t *val, int op)
{
        u32_t tmp;
        if(op == MII_WRITE) //write op
        {
                //put our payload in the MAC Management Transmit register
                ETH0->MACMTXD = ETH0->MACMTXD | (*val);
                //write operations to this register must be atomic.
                tmp     = (addr << 3 ) + 0x03; //0x3 = write + start
                ETH0->MACMCTL = (ETH0->MACMCTL&0xfb)|tmp;
        }
        else //read op
        {
                tmp     = (addr << 3) + 0x01; //0x1 = start, write is cleared.
                ETH0->MACMCTL = (ETH0->MACMCTL&0xfb)|tmp;
                //return the payload
                *val    = (ETH0->MACMRXD&0xffff);
        }
}

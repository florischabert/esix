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

#include "ethernet.h"
#include "mmap.h"
#include "uart.h"
#include <esix.h>

int frame[MAX_FRAME_SIZE]; //380 * 4bytes = 1520bytes

/**
 * ether_init : configures the ethernet hardware found in lm3s6965 
 */
void ether_init()
{
	SYSCTR->RCGC2	|= (1 << 30); //enable PHY clock 
	SYSCTR->RCGC2	|= (1 << 28); //enable MAC clock

	//give them a little time to wake up
	asm("nop");

	//set the MAC Management frequency (must be <2.5MHz,
	//Fmfq = Fcpu/(2*(MACDVR+1)) )
	//still dunno at what speed we're running. 50MHz now?
	ETH0->MACMDV	= 0x0000000a;

	//program a MAC address (we should be reading this from somewhere...)
	ETH0->MACIA0	= 0x003ae967;	//4 first MAC address bytes
	ETH0->MACIA1	|= 0x0000c58d;  //the first 2 bytes are marked as reserved

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

	//Unmask ethernet interrupt at the processor level and set
	//a priority of configMAX_SYSCALL_INTERRUPT_PRIORITY+2
	NVIC->IPR[10] &= ~(0xff << 16); // Set interrupt priority
	NVIC->IPR[10] |= ((configMAX_SYSCALL_INTERRUPT_PRIORITY+2) << 16);
	NVIC->ISER[1] |= (1 << 10); // Unmask ethernet interrupt

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
	ETH0->MACIM	|= 0x0000007f;

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
	ETH0->MACIM	|= 0x0000007f;
}

/**
 * ether_handler : Ethernet interrupt handler
 */
void ether_handler()
{
	u16_t int_val	= ETH0->MACRIS;

	if(int_val&RXINT)
		ether_frame_received();
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
}

/**
 * ether_frame_received : called after an interrupt has been received.
 * copies a frame from the RX ring buffer to a loacl buffer.
 * 
 */
void ether_frame_received()
{
	//int frame[380]; //380 * 4bytes = 1520bytes
	int i;
	int words_to_read;
	struct ether_frame_t *eth_f = frame;

	//wait for the frame to be fully buffered
	//while(!ETH0->MACNP);

	//read the first 4 bytes to get the frame length
	frame[0]	= ETH0->MACDATA;
	//convert the frame length (in bytes) to words (4 bytes)
	words_to_read	= (eth_f->FRAME_LENGTH/4);

	//we've read the first 4 bytes, now read FRAME_LENGTH/4 more words
	i=1;
	while((words_to_read-- > 0) 
		&& (i < MAX_FRAME_SIZE))
			frame[i++] = ETH0->MACDATA;

	//got a v6 frame, pass it to the v6 stack
	if(eth_f->ETHERTYPE == 0xdd86) // we are litle endian. In network order (big endian), it reads 0x86dd
		esix_received_frame((struct ip6_hdr *) &eth_f->data,
			(eth_f->FRAME_LENGTH-20));

	//automatically cleared when the RX FIFO is empty.
	//ETH0->MACRIS |= 0x00000001;
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

void ether_get_mac_addr(u16_t *val)
{
	val[0]	= (ETH0->MACIA0 >> 16); //we only want the first 2 bytes here
	val[1]	= (ETH0->MACIA0);	//this should get truncated so we're ok.
	val[2]	= (ETH0->MACIA1);
}
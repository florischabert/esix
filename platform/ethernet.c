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
	//j'avais 0x18 avant...
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
	ETH0->MACIM	&= ~0x0000007f;

	//Unmask interrupts at the MII level
	//u32_t mii_val = 0xffff;
	//ether_mii_request(0x11, &mii_val, MII_WRITE);
        ETH0->MACMTXD |= 0xff00;
	u16_t addr = 0x11;
        u16_t tmp     = (addr << 3 ) + 0x03; //0x3 = write + start
        ETH0->MACMCTL = tmp;

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
	u32_t int_val	= ETH0->MACRIS;
	GPIOF->DATA[1] ^= 1;

	switch(int_val)
	{
		case RXINT:
			ether_frame_received();
			break;
		case TXEMP:
			ether_txfifo_empty();
			break;
		case RXER:
			ether_rx_error();
			break;
		case TXER:
			ether_tx_error();
			break;
		case FOV: 
			ether_fifo_overrun();
			break;
		case PHYINT:
			ether_phy_int();
			break;
		case MDINT:
			ether_mii_transaction_complete();
			break;
		default :
			ether_big_fat_warning();
			break;
		ETH0->MACRIS &= ~0x0000007f;
	}
}

void ether_frame_received()
{
	uart_putc('P');
//	GPIOF->DATA[1] ^= 1;
}

void ether_txfifo_empty()
{
	
}

void ether_rx_error()
{
	
}

void ether_tx_error()
{
	
}

void ether_fifo_overrun()
{
	

}

void ether_phy_int()
{
	
}

void ether_mii_transaction_complete()
{
	
}

void ether_big_fat_warning(void)
{
	uart_putc('W');
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
                tmp     = (addr << 3) + 0x01; //0x1 = start, write is clea red.
                ETH0->MACMCTL = (ETH0->MACMCTL&0xfb)|tmp;
                //return the payload
                *val    = (ETH0->MACMRXD&0xffff);
        }
}


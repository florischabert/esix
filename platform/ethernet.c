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
	ETH0->MACMDV	= 0x00000001;

	//program a MAC address (we should be reading this from somewhere...)
	ETH->MACIA0	= 0x003ae967;	//4 first MAC address bytes
	ETH->MACIA1	|= 0x0000c58d;  //the first 2 bytes are marked as reserved

	//generate FCS in hardware, do autoneg, FD.
	ETH0->MACTCTL	|= 0x00000016;

	//flush rx fifo, reject frames with bad CRC in hardware
	ETH0->MACRCTL	|= 0x00000018;

	//tell the hardware to hand over multicast frames. We're going to need 
	//them for IPv6 and this ether chip doesn't have any mcast filter list.
	ETH0->MACRCTL	|= 0x00000002;
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
}

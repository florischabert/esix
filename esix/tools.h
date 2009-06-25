/**
 * @file
 * Useful stuff.
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

#ifndef _TOOLS_H
#define _TOOLS_H

#include "config.h"

//this is only for debug purposes
/**
 * GPIO register.
 */
	struct gpio_t
{
	u32_t DATA[256];
	u32_t DIR;
	u32_t IS;
	u32_t IBE;
	u32_t IEV;
	u32_t IM;
	u32_t RIS;
	u32_t MIS;
	u32_t ICR;
	u32_t AFSEL;
	u32_t Reserved0[55];
	u32_t DR2R;
	u32_t DR4R;
	u32_t DR8R;
	u32_t ODR;
	u32_t PUR;
	u32_t PDR;
	u32_t SLR;
	u32_t DEN;
	u32_t LOCK;
	u32_t CR;
};
#define GPIOF        ((volatile struct  gpio_t *) 0x40025000)
//debug


#define NULL ((void *) 0)

void esix_memcpy(void *dst, void *src, int len);

u16_t hton16(u16_t v);
u32_t hton32(u32_t v);
u16_t ntoh16(u16_t v);
u32_t ntoh32(u32_t v);


#endif

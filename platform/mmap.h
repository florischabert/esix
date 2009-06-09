/**
 * @file
 * LM3S6965 Memory map.
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

#ifndef _MEMORY_MAP
#define _MEMORY_MAP

#include "types.h"

// Structs for peripheral registers

/**
 * Nested Vectored Interrupt Controller register.
 */
struct nvic_t
{
	u32_t ISER[2];
	u32_t Reserved0[30];
	u32_t ICER[2];
  	u32_t Reserved1[30];
	u32_t ISPR[2];
	u32_t Reserved2[30];
	u32_t ICPR[2];
	u32_t Reserved3[30];
	u32_t IABR[2];
	u32_t Reserved4[62];
	u32_t IPR[15];
};

/**
 * System control register.
 */
struct sysctr_t
{
	u32_t DID0;
	u32_t DID1;
	u32_t DC0;
	u32_t Reserved0;
	u32_t DC1;
	u32_t DC2;
	u32_t DC3;
	u32_t DC4;
	u32_t Reserved1[4];
	u32_t PBORCTL;
	u32_t LDOPCTL;
	u32_t Reserved2[2];
	u32_t SRCR0;
	u32_t SRCR1;
	u32_t SRCR2;
	u32_t Reserved3;
	u32_t RIS;
	u32_t IMC;
	u32_t MISC;
	u32_t RESC;
	u32_t RCC;
	u32_t PLLCFG;
	u32_t Reserved4[2];
	u32_t RCC2;
	u32_t Reserved5[35];
	u32_t RCGC0;
	u32_t RCGC1;
	u32_t RCGC2;
	u32_t Reserved6;
	u32_t SCGC0;	
	u32_t SCGC1;	
	u32_t SCGC2;
	u32_t Reserved7;
	u32_t DCGC0;	
	u32_t DCGC1;	
	u32_t DCGC2;
	u32_t Reserved8[6];
	u32_t DSLPCLKCFG;
};

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

/**
 * UART register.
 */
struct uart_t {
	u32_t DR;
	u32_t RSR;
	u32_t Reserved0[4];
	u32_t FR;
	u32_t Reserved1;
	u32_t ILPR;
	u32_t IBRD;
	u32_t FBRD;
	u32_t LCRH;
	u32_t CTL;
	u32_t IFLS;
	u32_t IM;
	u32_t RIS;
	u32_t MIS;
	u32_t ICR;
};

// Memory map
#define NVIC         ((volatile struct nvic_t *) 0xe000e100)
#define SYSCTR       ((volatile struct sysctr_t *) 0x400fe000)
#define GPIOA        ((volatile struct gpio_t *) 0x40004000)
#define GPIOB        ((volatile struct gpio_t *) 0x40005000)
#define GPIOC        ((volatile struct gpio_t *) 0x40006000)
#define GPIOD        ((volatile struct gpio_t *) 0x40007000)
#define GPIOE        ((volatile struct gpio_t *) 0x40024000)
#define GPIOF        ((volatile struct gpio_t *) 0x40025000)
#define GPIOG        ((volatile struct gpio_t *) 0x40026000)
#define UART0        ((volatile struct uart_t *) 0x4000c000)

#endif
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

#define SYSCTR_BASE			0x400FE000
#define RIS						*((volatile unsigned*) (SYSCTR_BASE + 0x050))
#define RCC						*((volatile unsigned*) (SYSCTR_BASE + 0x060))
#define RCGC2					*((volatile unsigned*) (SYSCTR_BASE + 0x108))

#define GPIOA_BASE			0x40004000
#define GPIOB_BASE			0x40005000
#define GPIOC_BASE			0x40006000 
#define GPIOD_BASE			0x40007000 
#define GPIOE_BASE			0x40024000 
#define GPIOF_BASE			0x40025000 
#define GPIOG_BASE			0x40026000

#define GPIOF_DEN				*((volatile unsigned*) (GPIOF_BASE + 0x51c))
#define GPIOF_DIR				*((volatile unsigned*) (GPIOF_BASE + 0x400))

#endif
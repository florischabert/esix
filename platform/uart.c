/**
 * @file
 * UART driver.
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
#include "mmap.h"
#include "uart.h"

/**
 * UART0 initialization.
 */
void uart_init(void)
{
	u32_t reg;
	
	// IO configuration
	SYSCTR->RCGC2 |= (1 << 0); // Enable GPIOA clock
	asm("nop"); // FIXME
	
	GPIOA->AFSEL = (1 << 0) | (1 << 1); // PA0, PA1 are alternate functions
	GPIOA->DEN = (1 << 0) | (1 << 1);   // PA0, PA1 are digital

	
	// UART0 configuration
	SYSCTR->RCGC1 |= (1 << 0);  // Enable UART0 clock
	asm("nop"); // FIXME
	
	UART0->IBRD = 27;           // UART0 at 115.200 baud
	UART0->FBRD = 8;
	UART0->LCRH |= 3;           // 8 bits frame
	UART0->IM |= (1 << 4);      // Enable receive interrupt
	UART0->CTL |= 0;            // Enable the uart
	
	// Unmask interrupt line
	reg = NVIC->IPR[1];
	reg &= ~(0xff << 8);        // Set the interrupt priority
	reg |= ((configMAX_SYSCALL_INTERRUPT_PRIORITY + 1) << 8);
	NVIC->IPR[1] = reg;
	NVIC->ISER[0] |= (1 << 5);  // Enable UART0 interrupt line
}

/**
 * UART0 interrupt handler.
 */
void uart_handler(void)
{
	//GPIOF->DATA[1] = 1;
	// UARTMIS
	// UARTICR
}

/**
 * Send a byte through UART0.
 */
void uart_send_byte(u8_t byte)
{
	//UART0->IM |= (1 << 5); // Enable transmit interrupt
}

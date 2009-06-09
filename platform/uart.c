/**
 * @file
 * UART0 driver.
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
#include <queue.h>
#include "mmap.h"
#include "uart.h"

// Communication FIFOs
static xQueueHandle uart_send_queue;

/**
 * UART0 initialization.
 */
void uart_init(void)
{
	SYSCTR->RCGC1 |= (1 << 0); // Enable UART0 clock
	SYSCTR->RCGC2 |= (1 << 0); // Enable GPIOA clock

	GPIOA->AFSEL |= (1 << 0) | (1 << 1); // PA0,1 as alternate function
	GPIOA->ODR |= (1 << 0) | (1 << 1);   // Enable PA0,1 open drain
 
	UART0->IBRD = ((((50000000*8)/BAUD_RATE)+1)/2) / 64; // 115200 baud rate
	UART0->FBRD = ((((50000000*8)/BAUD_RATE)+1)/2) % 64;
	UART0->LCRH = (0x3 << 5);          // 8 bits frame
	UART0->CTL |= (1 << 8) | (1 << 9); // Enable TX, RX
	UART0->CTL |= (1 << 0);            // Enable UART0

	//UART0->IM |= (1 << 4); // Enable RX interrupt

	NVIC->IPR[1] &= ~(0xff << 8); // Set interrupt priority
	NVIC->IPR[1] |= ((configMAX_SYSCALL_INTERRUPT_PRIORITY+1) << 8);
	NVIC->ISER[0] |= (1 << 5); // Unmask UART0 interrupt
	
	uart_send_queue = xQueueCreate(32, sizeof(char));   
}

/**
 * UART0 interrupt handler.
 */
void uart_handler(void)
{
	static portBASE_TYPE reschedNeeded;
	char c;
	
	reschedNeeded = pdFALSE;
	
	// Data received
	if(UART0->MIS & (1 << 5))
	{
		if(xQueueReceiveFromISR(uart_send_queue, &c, &reschedNeeded) == pdTRUE)
			UART0->DR = c;
		else
		{
			UART0->IM &= ~(1 << 5); // Disable TX interrupt
			UART0->ICR = (1 << 5); // Disable TX interrupt
		}
	}
	portEND_SWITCHING_ISR(reschedNeeded);
}

/**
 * Send a character through UART0.
 */
void uart_putc(char c)
{
	//xQueueSend(uart_send_queue, &c, 0);	
	//UART0->IM |= (1 << 5); // Enable TX interrupt
	UART0->DR = c;
}

/**
 * Send a string through UART0.
 */
void uart_puts(char *s)
{
	while(*s)
		uart_putc(*s++);
}

/**
 * Print through UART0.
 */
int uart_printf(char *format, ...)
{
	return 0;
}

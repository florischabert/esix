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
#include <task.h>
#include <semphr.h>
#include "mmap.h"
#include "uart.h"

// Mutexes & FIFOs
static xSemaphoreHandle uart_send_sem;
static xQueueHandle uart_send_queue;

// Prototypes
void uart_send_task(void *param);

/**
 * UART0 initialization.
 */
void uart_init(void)
{
	SYSCTR->RCGC1 |= (1 << 0); // Enable UART0 clock
	SYSCTR->RCGC2 |= (1 << 0); // Enable GPIOA clock
	asm("nop");
	GPIOA->AFSEL |= (1 << 0) | (1 << 1); // PA0,1 as alternate function
	GPIOA->ODR |= (1 << 0) | (1 << 1);   // Enable PA0,1 open drain
 
	UART0->CTL &= ~(1 << 0);            // Disable UART0
	UART0->IBRD = ((((50000000*8)/BAUD_RATE)+1)/2) / 64; // 115200 baud rate
	UART0->FBRD = ((((50000000*8)/BAUD_RATE)+1)/2) % 64;
	UART0->LCRH = (0x3 << 5);          // 8 bits frame
	UART0->CTL |= (1 << 8) | (1 << 9); // Enable TX, RX
	UART0->CTL |= (1 << 0);            // Enable UART0

	//UART0->IM |= (1 << 4); // Enable RX interrupt
	UART0->IM |= (1 << 5); // Enable TX interrupt

	// Unmask processor interrupt line. We use  theFreeRTOS API in the handler
	// so the priority of the interrupt has to be lower than the priority of 
	// FreeRTOS interrupts
	NVIC->IPR[1] &= ~(0xff << 8); 
	NVIC->IPR[1] |= ((configMAX_SYSCALL_INTERRUPT_PRIORITY+1) << 8);
	NVIC->ISER[0] |= (1 << 5); // Unmask UART0 interrupt
	
	vSemaphoreCreateBinary(uart_send_sem);
	uart_send_queue = xQueueCreate(256, sizeof(char));
	xTaskCreate(uart_send_task, (signed char *) "uart send task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}

/**
 * UART0 send task.
 */
void uart_send_task(void *param)
{
	char c;
	
	while(1)
	{
		xQueueReceive(uart_send_queue, &c, portMAX_DELAY);
		UART0->DR = c;
		xSemaphoreTake(uart_send_sem, portMAX_DELAY);
	}
}

/**
 * UART0 interrupt handler.
 */
void uart_handler(void)
{
	static portBASE_TYPE resched_needed; 
	resched_needed = pdFALSE;
	
	// Transmit interrupt
	if(UART0->MIS & (1 << 5))
	{
		UART0->ICR |= (1 << 5); // Clear the interrupt
		xSemaphoreGiveFromISR(uart_send_sem, &resched_needed);
	}
	
	// If the released task has a higher priority than the current one, we 
	// reschedule the tasks. 
	portEND_SWITCHING_ISR(resched_needed);
}

/**
 * Send a character through UART0.
 */
void uart_putc(char c)
{
	xQueueSend(uart_send_queue, &c, 0);	
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
 * Print in hexadecimal.
 */
int uart_printx(unsigned int u)
{
	char s[11] = "0x00000000";
	unsigned int r;
	int i = 9;
	while(u)
	{
		r = u % 16;
		if(r >= 10)
			r += 'a' - '0' - 10;
		s[i--] = r + '0';
		u /= 16;
	}
	uart_puts(s);
	return 10;
}

/**
 * Print through UART0.
 */
int uart_printf(char *format, ...)
{
	char *arg;
	int n = 0;
	
	arg = (char *) &format + sizeof(format);
	while(*format != '\0')
	{
		if(*format == '%')
		{
			format++;
			if(*format == 0)
				break;
			switch(*format)
			{
				case '%':
					uart_putc('%');
					n++;
					break;
				case 'x':
					n += uart_printx(*(unsigned int *)((arg += sizeof(unsigned int)) - sizeof(unsigned int)));
					break;
			}
		}
		else
		{
			uart_putc(*format);
			n++;
		}
		format++;
	}
	return n;
}

/**
 * @file
 * Main code.
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
#include "mmap.h"
#include "uart.h"
#include "ethernet.h"
#include <esix.h>
// Prototypes
void hardware_init(void);
void led_task(void *param);
void led_task2(void *param);
int led2h;
/**
 * Main function.
 */	
void main(void)
{
	u16_t lla[3]; // MAC address
	lla[0] = 0x003a;
	lla[1] = 0xe967;
	lla[2] = 0xc58d;
	
	hardware_init();
	uart_init();
	ether_init(lla);
	ether_enable();
	esix_init(lla);
	
	// FreeRTOS tasks scheduling
	xTaskCreate(led_task, (signed char *) "led", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
	vTaskStartScheduler();
	while(1);
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName)
{
	//if(*pcTaskName == 'name')
		GPIOF->DATA[1] = 1;
}

extern int alloc_count;
/**
 * Toggle the LED
 */
void led_task(void *param)
{
	while(1)
	{
	//	uart_printf("task stack %x\n", uxTaskGetStackHighWaterMark(NULL));
		vTaskDelay(10000);
	}
}

/**
 * Setup the hardware.
 */
void hardware_init(void)
{
	u32_t reg;
   
	// Setup the main oscillator
	reg = SYSCTR->RCC;
	reg = (reg & ~(0xf << 23)) | (0x3 << 23); // Set sysclk divisor /4
	reg |= (0x1 << 22);                       // Enale sysclk divider 
	reg = (reg & ~(0xf << 6)) | (0xe << 6);   // 8MHz external quartz
	reg &= ~(0x3 << 4);                       // Main oscillator as input source
	reg &= ~(0x1 << 13);                      // Power on the PLL
	SYSCTR->RCC = reg;
	while(!(SYSCTR->RIS & (0x1 << 6)));       // Wait for the PLL to be stable
	SYSCTR->RCC &= ~(1 << 11);                // Enable the PLL as source

	// Setup the status LED
	SYSCTR->RCGC2 |= (1 << 5); // Enable GPIOF clock
	asm("nop");
	GPIOF->DIR = (1 << 0);    // PF0 as output
	GPIOF->DEN = (1 << 0);    // PF0 is digital
}

void toggle_led()
{
	GPIOF->DATA[1]	^= 1;	
}

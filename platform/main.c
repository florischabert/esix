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

// Prototypes
void hardware_init(void);
void led_task(void *param);

/**
 * Main function.
 *
 * Start the tasks.
 */
void main(void)
{
	hardware_init();
	
	// FreeRTOS tasks scheduling
	xTaskCreate(led_task, (signed char *) "main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
	vTaskStartScheduler();
}

/**
 * Toggle the LED
 */
void led_task(void *param)
{	
	while(1)
	{
		*((volatile unsigned*) (GPIOF_BASE + 0x004)) ^= 1;
		vTaskDelay(250);
	}
}

/**
 * Setup the hardware.
 */
void hardware_init(void)
{
	// Main oscillator configuration
	RCC = (RCC  & ~(0xf << 6)
			& ~(0x1 << 11)			// PLL is the syclk source
			& ~(0x3 << 4))			// MOSC is the oscillator source
			| (0xe << 6) 			// External crystal is at 8MHz
			| (0x1 << 1); 			// Enable the main oscillator
			
	while(!(RIS & (1 << 6)));	// Wait for the PLL to be stable
	
	RCC = (RCC & ~(0xf << 23))
			| (0x3 << 23)			// Sysclk at 50MHz
	      | (0x1 << 22)			// Enable sysclk divider
			| (0x1 << 1);			// Disable the internal oscillator
	
	// Peripheral clocks
	RCGC2 |= 0x1 << 5;			// Enable GPIOF
	asm("nop"); // FIXME
	
	// Status LED
	GPIOF_DEN |= 0x1;				// GF0 is digital 
	GPIOF_DIR |= 0x1;				// GF0 as output
}
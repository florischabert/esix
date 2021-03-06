/**
 * @file
 * Boot code.
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
#include "types.h"
#include "uart.h"
#include "ethernet.h"

// Prototypes
extern void main(void);
extern void vPortSVCHandler( void ); // FreeRTOS handler
extern void xPortPendSVHandler(void); // FreeRTOS handler
extern void xPortSysTickHandler(void); // FreeRTOS handler
void reset_handler(void);
void default_handler(void);

// LD script symbols
extern u32_t _etext, _data, _edata, _bss, _ebss;

// System stack
static u32_t system_stack[2048] __attribute__((section(".stack"))); // FIXME

//bootloader marker. placed at a fixed ram address and checked at each boot.
//if it equals "boot", then boot from ram directly, else boot from flash 
static u32_t boot_marker __attribute__((section(".boot_marker"), used))	= 0x626f6f74; //boot 

// Interrupt vector table
static void (*isr_handler[])() __attribute__((section(".isr_table"), used)) = 
{
	// Stack pointer
	(void *) (system_stack + sizeof(system_stack)),
	// Processor exceptions
	reset_handler,
	default_handler,      // NMI
	default_handler,      // Hard Fault
	default_handler,      // MPU mismatch
	default_handler,      // Bus Fault
	default_handler,      // Usage Fault
	0, 0, 0, 0,           // Reserved
	vPortSVCHandler,      // SVCall
	default_handler,      // Debug Monitor
	0,                    // Reserved
	xPortPendSVHandler,   // PendSV
	xPortSysTickHandler,  // SysTick
	// Interrupts
	default_handler,        // GPIO A
	default_handler,        // GPIO B
	default_handler,        // GPIO C
	default_handler,        // GPIO D
	default_handler,        // GPIO E
	uart_handler,           // UART 0
	default_handler,        // UART 1
	default_handler,        // SSI 0
	default_handler,        // I2C 0
	default_handler,        // PWM Fault
	default_handler,        // PWM 0
	default_handler,        // PWM 1
	default_handler,        // PWM 2
	default_handler,        // QEI 0
	default_handler,        // ADC 0
	default_handler,        // ADC 1
	default_handler,        // ADC 2
	default_handler,        // ADC 3
	default_handler,        // Watchdog
	default_handler,        // Timer 0 A
	default_handler,        // TImer 0 B
	default_handler,        // Timer 1 A
	default_handler,        // TImer 1 B
	default_handler,        // Timer 2 A
	default_handler,        // TImer 2 B
	default_handler,        // Analog Comp 0
	default_handler,        // Analog Comp 1
	0,                      // Reserved
	default_handler,        // System Control
	default_handler,        // Flash Control
	default_handler,        // GPIO F
	default_handler,        // GPIO G
	0,                      // Reserved
	default_handler,        // UART 2
	0,                      // Reserved
	default_handler,        // Timer 3 A
	default_handler,        // Timer 3 B
	default_handler,        // I2C 1
	default_handler,        // QEI 1
	0, 0, 0,                // Reserved
	ether_handler,          // Ethernet
	default_handler,        // Hibernation
};

/**
 * Reset handler.
 *
 * Copy the data segment to SRAM, 0-fill the bss and call the main function.
 */
__attribute__((naked))
void reset_handler(void)
{
	volatile u32_t *bm	= 0x200000f0;

	// Simple bootloader to boot from SRAM:
	
	//boot from RAM if we have "boot" just after the interrupt vector table
	/*if(*bm == 0x626f6f74)
	{
		*((volatile unsigned int *) 0xe000ed08) = 0x20000000; // Vector Table offset
		asm volatile ("mov r0, #0x20000000\n\t" 
		              "ldr sp, [r0]\n\t"    // Set the stack pointer
		              "ldr pc, [r0, #4]");  // Go to the reset_handler
	}
	*/

	u32_t *src, *dst;
	
	// Copy the data segment to SRAM
	src = &_etext;
	dst = &_data;
	while(dst < &_edata)
		*dst++ = *src++; 
		
	// Zero-fill the bss
	dst = &_bss;
	while(dst < &_ebss)
		*dst++ = 0;
		
	main();
}

/**
 * Default handler
 *
 * Infinite loop.
 */
void default_handler(void)
{
	while(1);
}

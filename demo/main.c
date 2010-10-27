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
#include <string.h>
#include <task.h>
#include "mmap.h"
#include "uart.h"
#include "ethernet.h"
#include <esix.h>
#include <socket.h>

// Prototypes
void hardware_init(void);
void main_task(void *param);
void http_server_task(void *param);
void tcp_server_task(void *param);
void tcp_chargen_task(void *param);
void udp_server_task(void *param);
void udp_echo_task(void *param);
void mdns_server_task(void *param);

extern struct esix_ipaddr_table_row *addrs;

u16_t lla2[3];

/**
 * Main function.
 */	
void main(void)
{
	u16_t lla[3]; // MAC address
	lla[0] = 0x003a;
	lla[1] = 0xe967;
	lla[2] = 0xc580;
	
	hardware_init();
	uart_init();
	ether_init(lla);
	ether_enable();

	lla[0] = HTON16(lla[0]);
	lla[1] = HTON16(lla[1]);
	lla[2] = HTON16(lla[2]);
	esix_init(lla);
	
	// FreeRTOS tasks scheduling
	xTaskCreate(main_task, (signed char *) "main", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(http_server_task, (signed char *) "http server", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(tcp_server_task, (signed char *) "tcp server", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(tcp_chargen_task, (signed char *) "tcp server", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(udp_server_task, (signed char *) "udp server", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(udp_echo_task, (signed char *) "udp echo server", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(mdns_server_task, (signed char *) "mdns server", 200, NULL, tskIDLE_PRIORITY + 1, NULL);
	vTaskStartScheduler();
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName)
{
	GPIOF->DATA[1] = 1;
}

void main_task(void *param)
{	
	while(1)
	{
		//uart_printf("task stack %x\n", uxTaskGetStackHighWaterMark(NULL));
		vTaskDelay(1000);
		esix_periodic_callback();
	}
}

void udp_server_task(void *param)
{
	static char buff[100];
	int soc;
	struct sockaddr_in6 from, to;
	int sockaddrlen = sizeof(struct sockaddr_in6);
	
	to.sin6_port = HTON16(2009);
	to.sin6_addr = in6addr_any;
	
	soc = socket(AF_INET6, SOCK_DGRAM, 0);
	bind(soc, &to, sockaddrlen);
	listen(soc, 1);
		
	while(1)
	{
		vTaskDelay(100);
		if(recvfrom(soc, buff, 99, 0, &from, &sockaddrlen) <0)
			continue;
		if(strncmp(buff, "toggle_led\n", 11))
			sendto(soc, "commands : toggle_led\n", 22, 0, &from, sizeof(struct sockaddr_in6));
		else
		{
			sendto(soc, "status led toggled.\n", 20, 0, &from, sizeof(struct sockaddr_in6));
			GPIOF->DATA[1]	^= 1;
		}
	}
}

void udp_echo_task(void *param)
{
	static char buff[100];
	int soc, nbread;
	struct sockaddr_in6 from, to;
	int sockaddrlen = sizeof(struct sockaddr_in6);
	
	to.sin6_port = HTON16(7);
	to.sin6_addr = in6addr_any;
	
	if((soc = socket(AF_INET6, SOCK_DGRAM, 0)) <0)
		uart_printf("socket failed");
	if(bind(soc, &to, sockaddrlen) < 0)
		uart_printf("bind failed");
	if(listen(soc, 1) <0)
		uart_printf("listen failed");
		
	while(1)
	{
		vTaskDelay(50);
		if((nbread = recvfrom(soc, buff, 99, 0, &from, &sockaddrlen)) >=0)
			sendto(soc, buff, nbread, 0, &from, sizeof(struct sockaddr_in6));
	}
}

void tcp_server_task(void *param)
{
	static char buff[100];
	int soc, conn, len;
	struct sockaddr_in6 serv;
	int sockaddrlen = sizeof(struct sockaddr_in6);
	
	serv.sin6_port = HTON16(2010);
	serv.sin6_addr = in6addr_any;
	
	if((soc = socket(AF_INET6, SOCK_STREAM, 0)) <0)
		uart_printf("tcp_server : socket failed\n");
	if((bind(soc, &serv, sockaddrlen))<0)
		uart_printf("tcp_server : bind failed\n");
	if(listen(soc, 1)<0)
		uart_printf("tcp_server : listen failed\n");

	while(1)
	{
		vTaskDelay(500);
		if((conn = accept(soc, NULL, NULL)) <0)
			continue;

		send(conn, "hey!\ncommands : toggle_led, help, exit\n", 39, 0);
		send(conn, "> ", 2, 0);
		
		while(1)
		{
			vTaskDelay(100);
			if((len = recv(conn, buff, 99, 0)) <0)
			{
				if(len == -1)
					continue;
				else
					break;
			}

			send(conn, buff, len, 0);
			if(!strncmp(buff, "exit", 4))
			{
				break;
			}
			else if(!strncmp(buff, "toggle_led", 10))
			{
				send(conn, "status led toggled.\n", 20, 0);
				GPIOF->DATA[1]	^= 1;
			}
			else if(!strncmp(buff, "help", 4))
			{
				send(conn, "commands : toggle_led, help, exit\n", 34, 0);
			}
			send(conn, "> ", 2, 0);
		}
		close(conn);
	}
}

void tcp_chargen_task(void *param)
{
	static char buff[1400];
	int soc, conn, len;
	char* a = buff + 1398;
	struct sockaddr_in6 serv;
	int sockaddrlen = sizeof(struct sockaddr_in6);

	while(a-- > buff)
		*a = '$';
	
	serv.sin6_port = HTON16(19);
	serv.sin6_addr = in6addr_any;
	
	if((soc = socket(AF_INET6, SOCK_STREAM, 0)) <0)
		uart_printf("tcp_server : socket failed\n");
	if((bind(soc, &serv, sockaddrlen))<0)
		uart_printf("tcp_server : bind failed\n");
	if(listen(soc, 1)<0)
		uart_printf("tcp_server : listen failed\n");

	while(1)
	{
		vTaskDelay(500);
		if((conn = accept(soc, NULL, NULL)) <0)
			continue;

		send(conn, "chargen starting...\n", 21, 0);
		vTaskDelay(500);
		
		while(1)
		{
			vTaskDelay(50);
			if((send(conn, buff, 1398, 0))<0)
				break;
		}
		close(conn);
	}
}

void http_server_task(void *param)
{
	static char buff[800];
	int soc, conn, len;
	struct sockaddr_in6 serv;
	int sockaddrlen = sizeof(struct sockaddr_in6);
	static char *toggle = "HTTP/1.1 200 OK\nServer: quick-and-dirty\nContent-Length: 133\nConnection: close\nContent-Type: text/html;\n\n<html>The LED is now     \n<br />\n<form name=\"input\" action=\"/toggle\" method=\"get\">\n<input type=\"submit\" value=\"Toggle LED\" /></form>\n</html>";
	
	serv.sin6_port = HTON16(80);
	serv.sin6_addr = in6addr_any;
	
	if((soc = socket(AF_INET6, SOCK_STREAM, 0)) <0)
		uart_printf("tcp_server : socket failed\n");
	if((bind(soc, &serv, sockaddrlen))<0)
		uart_printf("tcp_server : bind failed\n");
	if(listen(soc, 1)<0)
		uart_printf("tcp_server : listen failed\n");
		
	while(1)
	{
		//remember all our socket primitives are
		//nonblocking for now
		vTaskDelay(500);
		if((conn = accept(soc, NULL, NULL)) <0)
			continue;

		while(1)
		{
			vTaskDelay(300);
			if((len = recv(conn, buff, 799, 0)) <0)
			{
				if(len == -1)
					continue;
				else
					break;
			}

			if(!strncmp(buff, "GET ", 4))
			{
				if(!strncmp(buff, "GET /toggle", 10))
					GPIOF->DATA[1]	^= 1;	

				if(GPIOF->DATA[1]	== 1)
					strncpy(toggle+125, "on. ", 4);
				else
					strncpy(toggle+125, "off.", 4);
				
				send(conn, toggle, strlen(toggle), 0);
			}
			break;
		}
		close(conn);		
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
	reg |= (0x1 << 22);                       // Enable sysclk divider 
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

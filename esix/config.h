/**
 * @file
 * Comment
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

#ifndef _CONFIG_H
#define _CONFIG_H

#define ESIX_MAX_IPADDR	8 	//max number of IP addresses the node can have
#define ESIX_MAX_RT	8 	//max number of routes the node can have
#define ESIX_MAX_NB 16 //max number of neighbors in the table
#define ESIX_MAX_SOCK 32 //max number of sockets
#define FIRST_PORT 32000 //make sure that ESIX_MAX_SOCK < LAST_PORT - FIRST_PORT
#define LAST_PORT  65535 //or mayhem will happen
#define ESIX_QUEUE_DEPHT 5 //per-socket packet queue depht (received + send (for tcp))

#define INTERFACE	0 //default interface # until we have a proper intf
				//management system.
#define MAX_RETX_TIME 120

#define DEFAULT_TTL		64 	//default TTL when unspecified by
						//router advertisements
#define DEFAULT_MTU		1500
	
				
typedef unsigned long long u64_t;
typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t; 
typedef unsigned int size_t; 

//endianess...
#define LITTLE_ENDIAN 

#endif

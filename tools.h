/**
 * @file
 * Useful stuff.
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

#ifndef _TOOLS_H
#define _TOOLS_H

#include "config.h"
#include "include/esix.h"

#include <stdint.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define esix_foreach(item_ptr, array) \
	for (item_ptr = &array[0]; item_ptr < array + sizeof(array)/sizeof(array[0]); item_ptr++)

typedef struct esix_list {
	struct esix_list *prev;
	struct esix_list *next;
} esix_list;

#define esix_list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define esix_list_init(head) \
	do { \
		(head)->prev = (head); \
		(head)->next = (head); \
	} while (0)

#define esix_list_add(new, head) \
	do { \
		(head)->next->prev = (new); \
		(new)->next = (head)->next; \
		(new)->prev = (head); \
		(head)->next = (new); \
	} while (0)

#define esix_list_del(list) \
	do { \
		(list)->prev->next = (list)->next; \
		(list)->next->prev = (list)->prev; \
		(list)->next = (list); \
		(list)->prev = (list); \
	} while (0)

#define esix_list_foreach(item, head, link) \
	for (item = esix_list_entry((head)->next, typeof(*item), link); item->link.next != (head)->next; item = esix_list_entry(item->link.next, typeof(*item), link))

void esix_memcpy(void *dst, const void *src, int len);
int esix_memcmp(const void *p1, const void *p2, int len);
int esix_strlen(const char *s);

uint32_t esix_get_time();

uint16_t hton16(uint16_t v);
uint32_t hton32(uint32_t v);
#define ntoh16 hton16
#define ntoh32 hton32

#endif

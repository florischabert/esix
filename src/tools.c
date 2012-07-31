/**
 * @file
 * useful stuff.
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

#include "tools.h"

/*
 * Copy len bytes from src to dst.
 */
void esix_memcpy(void *dst, const void *src, int len)
{
	char *bdst;
	const char *bsrc;
	uint32_t *wdst = dst;
	const uint32_t *wsrc = src;

	for(; len >= 4; len -= 4) // TODO: optimize for other bus width
		*wdst++ = *wsrc++;
	bdst = (char *) wdst;
	bsrc = (char *) wsrc;
	while(len--)
		*bdst++ = *bsrc++;
}

/*
 * Compare the len first bytes.
 */
int esix_memcmp(const void *p1, const void *p2, int len)
{
	const char *pb1 = p1;
	const char *pb2 = p2;

	while(len--)
	{
		if(*pb1 != *pb2)
			return *pb2 - *pb1;
		pb1++;
		pb2++;
	}	
	return 0;
}

int esix_strlen(const char *s)
{
	int i = 0;

	if (!s) {
		return 0;
	}

	while (*s++) {
		i++;
	}

	return i;
}

/**
 * hton16 : converts host endianess to big endian (network order) 
 */
uint16_t hton16(uint16_t v)
{
	unsigned char *cv = (unsigned char *)&v;
	return ((uint16_t)cv[0] << 8 ) |
	       ((uint16_t)cv[1] << 0 );
}

/**
 * hton32 : converts host endianess to network order
 */
uint32_t hton32(uint32_t v)
{
	unsigned char *cv = (unsigned char *)&v;
	return ((uint32_t)cv[0] << 24) |
	       ((uint32_t)cv[1] << 16) |
	       ((uint32_t)cv[2] << 8 ) |
	       ((uint32_t)cv[3] << 0 );
}

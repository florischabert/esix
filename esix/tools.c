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
	u32_t *wdst = dst;
	const u32_t *wsrc = src;
	
	for(; len >= 4; len -= 4) // TODO: optimize for other bus width
		*wdst++ = *wsrc++;
	bdst = (char *) wdst;
	bsrc = (char *) wsrc;
	while(len--)
		*bdst++ = *bsrc++;
}

/**
 * hton16 : converts host endianess to big endian (network order) 
 */
u16_t hton16(u16_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 8) & 0xff00) | ((v >> 8) & 0x00ff);
}

/**
 * hton32 : converts host endianess to big endian (network order) 
 */
u32_t hton32(u32_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 24) & 0xff000000) |
	       ((v << 8) & 0x00ff0000) |
	       ((v >> 8) & 0x0000ff00) |
	       ((v >> 24) & 0x000000ff);
}

/**
 * ntoh16 : converts network order to host endianess
 */
u16_t ntoh16(u16_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 8) & 0xff00) | ((v >> 8) & 0x00ff);
}

/**
 * ntoh32 : converts network order to host endianess 
 */
u32_t ntoh32(u32_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 24) & 0xff000000) |
	       ((v << 8) & 0x00ff0000) |
	       ((v >> 8) & 0x0000ff00) |
	       ((v >> 24) & 0x000000ff);
}

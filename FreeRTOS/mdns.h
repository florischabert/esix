#ifndef _MDNS_H
#define _MDNS_H
#include <string.h>
#include <esix.h>

#define HOSTNAME "bugs.local"
#define HOSTNAME_LEN 11


void mdns_server_task(void *param);

struct dns
{
	u16_t id;
	u16_t flags;
	u16_t questions;
	u16_t answers;
	u16_t authority;
	u16_t additionnal;
	u8_t hostname_len;
	char hostname[HOSTNAME_LEN];
	u16_t type;
	u16_t _class;
	u32_t ttl;
	u16_t datalen;
	struct ip6_addr addr;	
} __attribute__((__packed__));

#endif

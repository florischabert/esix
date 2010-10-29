#include "../esix/intf.h"
#include "../esix/include/socket.h"
#include "mdns.h"


void mdns_server_task(void *param)
{
	//THIS WAS ONLY WRITTEN FOR TEST PURPOSES
	//USE IT AT YOUR OWN RISK (well, actually, don't use
	//it at all...) 

	static char buff[256];
	int soc, nbread, i;
	struct sockaddr_in6 from, to;
	struct ip6_addr mcast;
	
	int sockaddrlen = sizeof(struct sockaddr_in6);
	struct dns r;
	struct dns *q;	

	to.sin6_port = hton16(5353);
	esix_memcpy(&to.sin6_addr, &in6addr_any, 16);

		vTaskDelay(600);

	//ok this is plain wrong, but needed to receive multicast traffic
	mcast.addr1 = hton32(0xff020000);
	mcast.addr2 = 0;
	mcast.addr3 = 0;
	mcast.addr4 = hton32(0xfb);
	esix_intf_add_address(&mcast, 0x80, 0, MULTICAST); 

	//grab an address
	if((i=esix_intf_get_type_address(GLOBAL)) < 0)
		i=esix_intf_get_type_address(LINK_LOCAL);


	if((soc = socket(AF_INET6, SOCK_DGRAM, 0)) <0)
		uart_printf("mdns task : socket failed\n");
	if(bind(soc, &to, sockaddrlen) < 0)
		uart_printf("mdns task : bind failed\n");
	if(listen(soc, 1) <0)
		uart_printf("mdns task : listen failed\n");

	to.sin6_port = hton16(5353);
	esix_memcpy(&to.sin6_addr, &mcast, 16);

	esix_memcpy(&r.hostname, HOSTNAME, HOSTNAME_LEN);
	r.hostname[HOSTNAME_LEN-7] = 0x05;
	r.flags = 0x84;
	r.questions = 0;
	r.answers = hton16(1);
	r.authority = 0;
	r.additionnal = 0;
	r.hostname_len = HOSTNAME_LEN-7;
	r.type = hton16(0x1c);
	r._class = hton16(0x8001);
	r.ttl  = hton32(0x14);
	r.datalen = hton16(0x10);
	esix_memcpy(&r.addr, &addrs[i]->addr, 16);
		
	while(1)
	{
		vTaskDelay(400);
		if((nbread = recvfrom(soc, buff, 255, 0, &from, &sockaddrlen)) <0)
			continue;
		
		q = (struct dns*) &buff;
		if(q->flags == 0 && q->questions != 0
			&& q->answers == 0x00 && q->authority == 0
			&& q->additionnal == 0 && esix_memcmp(&r.hostname, (u8_t*) q+13, HOSTNAME_LEN-1) == 0 )
		{
			r.id = q->id;

			sendto(soc, &r, sizeof(struct dns), 0, &to, sizeof(struct sockaddr_in6));
		}
	}
}

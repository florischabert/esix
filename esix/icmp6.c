/**
 * @file
 * esix stack, icmp6 processing functions.
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

#include "icmp6.h"
#include "tools.h"
#include "intf.h"

/**
 * Handles icmp packets.
 */
void esix_icmp_process(struct icmp6_hdr *icmp_hdr, int length, struct ip6_hdr *ip_hdr )
{
	//check if we have enough bytes to read the ICMP header
	if(length < 4)
		return;

	//determine what to do next
	switch(icmp_hdr->type)
	{
		case NBR_SOL:
			esix_icmp_process_neighbor_sol(
				(struct icmp6_neighbor_sol *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		case RTR_ADV:
			esix_icmp_process_router_adv(
				(struct icmp6_router_adv *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		case ECHO_RQ: 
			esix_icmp_process_echo_req(
				(struct icmp6_echo_req *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		default:
			uart_printf("Unknown ICMP packet, type: %x\n", icmp_hdr->type);
	}
}

/*
 * Send an ICMPv6 packet
 */
void esix_icmp_send(struct ip6_addr *saddr, struct ip6_addr *daddr, u8_t hlimit, u8_t type, u8_t code, void *data, u8_t len)
{
	struct icmp6_hdr *hdr = esix_w_malloc(sizeof(struct icmp6_hdr) + len);
	
	hdr->type = type;
	hdr->code = code;
	hdr->chksum = 0;
	esix_memcpy(hdr + 1, data, len);
	esix_w_free(data);	
	
	hdr->chksum = hton16(esix_icmp_compute_checksum(saddr, daddr, hdr, len + sizeof(struct icmp6_hdr)));
	
	esix_ip_send(saddr, daddr, hlimit, ICMP, hdr, len + sizeof(struct icmp6_hdr));
}

u16_t esix_icmp_compute_checksum(struct ip6_addr *saddr, struct ip6_addr *daddr, void *data, u8_t len)
{ // FIXME: it just doesn't work
	u16_t *hw = data;
	u16_t sum = 0;
	
	// IPv6 pseudo-header byte sum: IP addresses, payload len, next header = 58
	/*for(i = 0; i < 8; i++)
	{
		sum += *(((u16_t *) saddr)+i);
		sum += *(((u16_t *) daddr)+i);
	}*/
	sum += len;
	sum += 58;
	
	// ICMP message byte sum
	for(; len; len -= 2)
		sum += *hw++;
	
	while(sum >> 16)
		sum = (sum >> 16) + (sum & 0xffff);
	
	return 0xd64f;
	return ~sum;
}

/**
 * Sends a TTL expired message back to its source.
 */
void esix_icmp_send_ttl_expired(struct ip6_hdr *hdr)
{
}

/**
 * Crafts and sends a router sollicitation
 * on the interface specified by index. 
 */
void esix_icmp_send_router_sol(int intf_index)
{
	int i;
	struct ip6_addr dest;
	dest.addr1	= hton32(0xff020000);
	dest.addr2	= hton32(0x00000000);
	dest.addr3	= hton32(0x00000000);
	dest.addr4	= hton32(0x00000002);
	
	u8_t len = sizeof(struct icmp6_router_sol) + sizeof(struct icmp6_opt_lla);
	struct icmp6_router_sol *ra_sol = esix_w_malloc(len);
	struct icmp6_opt_lla *opt = (struct icmp6_opt_lla *) (ra_sol + 1);

	opt->type	= S_LLA;
	opt->len8	= 1; //1 * 8 bytes
	for(i=0; i<3; i++)
		opt->lla[i]	= neighbors[0]->lla[i];

	esix_icmp_send(&addrs[0]->addr, &dest, 255, RTR_SOL, 0, ra_sol, len);
}

/*
 * Process a neighbor solicitation
 */
void esix_icmp_process_neighbor_sol(struct icmp6_neighbor_sol *nb_sol, int len, struct ip6_hdr *hdr)
{
	int i;

	// FIXME: i can't do that
	i = esix_intf_get_neighbor_index(&hdr->saddr, INTERFACE);
	if(i < 0) // the neighbor isn't in the cache, we add it
		esix_intf_add_neighbor(&hdr->saddr, ((struct icmp6_opt_lla *) (nb_sol + 1))->lla, 0, INTERFACE);
		
	i = esix_intf_get_address_index(&nb_sol->target_addr, ANY_SCOPE, ANY_MASK);	
	if(i >= 0) // we're solicited, we now send an advertisement
		esix_icmp_send_neighbor_adv(&nb_sol->target_addr, &hdr->saddr, 1);
}

/*
 * Process an ICMPv6 Echo Request.
 */
void esix_icmp_process_echo_req(struct icmp6_echo_req *echo_rq, int length, struct ip6_hdr *ip_hdr)
{
	uart_printf("ethernet stack %x\n", uxTaskGetStackHighWaterMark(NULL));
	uart_puts("echo rq received\n");
}

/*
 * Send a neighbor advertisement.
 */
void esix_icmp_send_neighbor_adv(struct ip6_addr *saddr, struct ip6_addr *daddr, int is_solicited)
{
	u8_t len = sizeof(struct icmp6_neighbor_adv) + sizeof(struct icmp6_opt_lla);
	struct icmp6_neighbor_adv *nb_adv = esix_w_malloc(len);
	struct icmp6_opt_lla *opt = (struct icmp6_opt_lla *) (nb_adv + 1);
	
	nb_adv->r_s_o_reserved = hton32(is_solicited << 30);
	nb_adv->target_addr = *saddr;
	
	opt->type = 2; // Target Link-Layer Address
	opt->len8 = 1; // length: 1x8 bytes
	opt->lla[0] = hton16(neighbors[0]->lla[0]);
	opt->lla[1] = hton16(neighbors[0]->lla[1]);
	opt->lla[2] = hton16(neighbors[0]->lla[2]);

	esix_icmp_send(saddr, daddr, 255, NBR_ADV, 0, nb_adv, len);
}

/**
 * Parses RA messages, add/update a default route,
 * a prefix route and builds an IP address out of this prefix.
 */
void esix_icmp_process_router_adv(struct icmp6_router_adv *rtr_adv, int length,
	 struct ip6_hdr *ip_hdr)
{
	struct ip6_addr addr, addr2;
	int i=0;
	u32_t mtu;
	struct icmp6_opt_prefix_info *pfx_info = NULL;
	struct icmp6_opt_mtu *mtu_info;
	struct icmp6_option_hdr *option_hdr;
	int got_prefix_info	= 0;

	//we at least need to have 16 bytes to parse...
	//(router advertisement without any option = 16 bytes,
	//advertises only a default route)
	if(ntoh16(length) < 16 ) 
		return;

	mtu	= DEFAULT_MTU;
				
	//parse options like MTU and prefix info
	i=12; 	//we at least need 2 more bytes (type + length) to be able to process
			//the first option field (those are TLVs)
	while (i + 2 < length) // is the ip packet long enough to continue?
	{
		option_hdr = (struct icmp6_option_hdr *) ((u8_t *) rtr_adv + i);
		switch(option_hdr->type)
		{
			case PRFX_INFO:
				pfx_info = (struct icmp6_opt_prefix_info *) &option_hdr->payload; 
				//the advertised prefix length MUST be 64 (0x40) in order to do
				//stateless autoconfigration.
				if( (i+32) < ntoh16(length) && pfx_info->prefix_length == 0x40)
				{
					got_prefix_info	= 1;
				}
				i+=	32; 
			break;

			case MTU:
				toggle_led();
				mtu_info = (struct icmp6_opt_mtu *) &option_hdr->payload; 
				if( (i+8) < ntoh16(length) )
				{
					mtu	= ntoh32(mtu_info->mtu);
				}
				i+=8;
			break;

			default:
				i+= (option_hdr->length*8); //option_hdr->length gives the size of
								//type + length + data fields in
								//8 bytes multiples
								//skip this length since we don't know
								//how to process it
			break; 	
		}
	}

	//now add the routes (we needed the MTU value first)

	//default route
	//a lifetime of 0 means remove the route
	if(ntoh32(rtr_adv->rtr_lifetime) == 0x0)
	{
		//TODO delete the default route here.
	}
	else
	{
		addr.addr1 = 0;
		addr.addr2 = 0;
		addr.addr3 = 0;
		addr.addr4 = 0;
		esix_intf_add_route(&addr, 	//default dest
					0x0,			//default mask
					&ip_hdr->saddr,		//next hop
					esix_w_get_time() + ntoh32(rtr_adv->rtr_lifetime), //exp. date
					rtr_adv->cur_hlim,	//TTL
					mtu,
					INTERFACE);
	}


	//global address && onlink route
	//a lifetime of 0 means remove the address/route
	if(got_prefix_info && pfx_info->valid_lifetime == 0x0)
	{
		//TODO : remove the address / route here
	}
	else if (got_prefix_info)
	{
		//builds a new global scope address
		//not endian-safe for now, words in the prefix field
		//are not aligned when received
		addr.addr1 = hton32(pfx_info->p[0] << 24
					| pfx_info->p[1] << 16 
					| pfx_info->p[2] << 8 
					| pfx_info->p[3]);

		addr.addr2 = hton32(pfx_info->p[4] << 24 
					| pfx_info->p[5] << 16 
					| pfx_info->p[6] << 8 
					| pfx_info->p[7]); // FIXME: mac adress in the table ?

		addr.addr3 = hton32(((neighbors[0]->lla[0] << 16) & 0xff0000)
			| (neighbors[0]->lla[1] & 0xff00)
			| 0x020000ff); //stateless autoconf, 0x02 : universal bit

		addr.addr4 = hton32((0xfe000000) //0xfe here is OK
			| ((neighbors[0]->lla[1] << 16) & 0xff0000) 
			| neighbors[0]->lla[2]);

		esix_intf_add_address(&addr,
				0x40,				// /64
				esix_w_get_time() + pfx_info->valid_lifetime, //expiration date
				GLOBAL_SCOPE);

		addr.addr1 = hton32(pfx_info->p[0] << 24	//prefix 
					 | pfx_info->p[1] << 16 
					 | pfx_info->p[2] << 8
					 | pfx_info->p[3]);
		addr.addr2 = 			hton32(pfx_info->p[4] << 24 	//prefix
					 | pfx_info->p[5] << 16 
					 | pfx_info->p[6] << 8
					 | pfx_info->p[7]);
		addr.addr3 = 0;
		addr.addr4 = 0;
		addr2.addr1 = 0;
		addr2.addr2 = 0;
		addr2.addr3 = 0;
		addr2.addr4 = 0;
		//onlink route (local route for our own subnet)
		esix_intf_add_route(&addr,
					0x40,				//ALWAYS /64 for autoconf.
					&addr2,
					esix_w_get_time() + ntoh32(pfx_info->valid_lifetime), //exp. date
					rtr_adv->cur_hlim,	//TTL
					mtu,
					INTERFACE);
	}//else if (got_prefix_info)
}


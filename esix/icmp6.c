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
 * esix_received_icmp : handles icmp packets.
 */
void esix_icmp_received(struct icmp6_hdr *icmp_hdr, int length, struct ip6_hdr *ip_hdr )
{
	//check if we have enough bytes to read the ICMP header
	if(length < 4)
		return;

	//determine what to do next
	switch(icmp_hdr->type)
	{
		case NBR_SOL:
			break;
		case RTR_ADV:
			esix_icmp_parse_rtr_adv(
				(struct icmp6_rtr_adv *) &icmp_hdr->data, length - 4, ip_hdr);
			break;
		case ECHO_RQ: 
			break;
		default:	
			return;
	}
}


/**
 * esix_send_ttl_expired : sends a TTL expired message
 * back to its source.
 */
void	esix_icmp_send_ttl_expired(struct ip6_hdr *hdr)
{
}

/**
 * esix_send_router_sollicitation : crafts and sends a router sollicitation
 * on the interface specified by index. 
 */
void	esix_icmp_send_router_sollicitation(int intf_index)
{
}

/**
 * esix_parse_rtr_adv : parses RA messages, add/update a default route,
 * a prefix route and builds an IP address out of this prefix.
 */
void esix_icmp_parse_rtr_adv(struct icmp6_rtr_adv *rtr_adv, int length,
	 struct ip6_hdr *ip_hdr)
{
	int i=0;
	struct icmp6_opt_prefix_info *pfx_info;
	struct icmp6_opt_mtu *mtu_info;
	struct icmp6_option_hdr *option_hdr;
	struct esix_route_table_row *default_rt	= NULL;

	//we at least need to have 16 bytes to parse...
	//(router advertisement without any option = 16 bytes,
	//advertises only a default route)
	if(ntoh16(length) < 16 ) 
		return;

	//look up the routing table to see if this route already exists.
	//if it does, select it instead of creating a new one.
	

	while(i<ESIX_MAX_RT)
	{
		if((routes[i] != NULL) &&
			(routes[i]->addr.addr1		== 0x0) &&
			(routes[i]->addr.addr2		== 0x0) &&
			(routes[i]->addr.addr3		== 0x0) &&
			(routes[i]->addr.addr4		== 0x0) &&
			(routes[i]->next_hop.addr1	== ip_hdr->saddr1) &&
			(routes[i]->next_hop.addr2	== ip_hdr->saddr2) &&
			(routes[i]->next_hop.addr3	== ip_hdr->saddr3) &&
			(routes[i]->next_hop.addr4	== ip_hdr->saddr4))

			default_rt	= routes[i];
		i++;
	}

	if(default_rt == NULL)
	{
		default_rt = esix_w_malloc(sizeof(struct esix_route_table_row));
		esix_intf_add_to_active_routes(default_rt);  //add the pointer now to avoid adding it
							//twice when we're updating
	}
	//TODO : check if malloc succeeded

	
	default_rt->addr.addr1		= 0x0;
	default_rt->addr.addr2		= 0x0;
	default_rt->addr.addr3		= 0x0;
	default_rt->addr.addr4		= 0x0;
	default_rt->mask		= 0;
	default_rt->next_hop.addr1	= ip_hdr->saddr1;
	default_rt->next_hop.addr2	= ip_hdr->saddr2;
	default_rt->next_hop.addr3	= ip_hdr->saddr3;
	default_rt->next_hop.addr4	= ip_hdr->saddr4;
	default_rt->ttl			= rtr_adv->cur_hlim;
	default_rt->mtu			= DEFAULT_MTU;
	default_rt->expiration_date	= esix_w_get_time() + ntoh32(rtr_adv->rtr_lifetime);
	default_rt->interface		= INTERFACE;

	//parse options like MTU and prefix info
	i=16+2; 	//we at least need 2 more bytes (type + length) to be able to process
			//the first option field (those are TLVs)
	option_hdr = &rtr_adv->option_hdr;
	while (i < length) // is the ip packet long enough to continue?
	{
		switch(option_hdr->type)
		{
			case PRFX_INFO:
				pfx_info = (struct icmp6_opt_prefix_info *) &option_hdr->payload; 
				//the advertised prefix length MUST be 64 (0x40) in order to do
				//stateless autoconfigration.
				if( (i+30) < ntoh16(length) && pfx_info->prefix_length != 0x40)
				{
					//builds a new global scope address
					//not endian-safe for now, words in the prefix field
					//are not aligned when received
					esix_intf_new_addr(  hton32(pfx_info->p[0] << 24	//prefix 
								| pfx_info->p[1] << 16 
								| pfx_info->p[2] << 8
								| pfx_info->p[3]),
							hton32(pfx_info->p[4] << 24 	//prefix
								| pfx_info->p[5] << 16 
								| pfx_info->p[6] << 8
								| pfx_info->p[7]),
								hton32((mac_addr.h << 16) | ((mac_addr.l >> 16) & 0xff00) | 0xff),	//stateless autoconf FIXME: universal bit ?
								hton32((0xfe << 24) | (mac_addr.l & 0xffffff)),
							0x40,				// /64
							esix_w_get_time() + pfx_info->valid_lifetime, //expiration date
							GLOBAL_SCOPE);
				}//if(i+...
				i+=	30; 
				option_hdr	= (struct icmp6_option_hdr*)(((char*) rtr_adv)+i);
			break;

			case MTU:
				mtu_info = (struct icmp6_opt_mtu *) &rtr_adv->option_hdr.payload; 
				if( (i+6) < ntoh16(length))
				{
					default_rt->mtu	= ntoh16(mtu_info->mtu);
				}
				i+=8;
				option_hdr	= (struct icmp6_option_hdr*) ((char*) rtr_adv)+i;
			break;

			default:
				i+= (option_hdr->length*8) - 2; //option_hdr->length gives the size of
								//type + length + data fields in
								//8 bytes multiples
								//skip this length since we don't know
								//how to process it
				option_hdr	= (struct icmp6_option_hdr*) ((char*) rtr_adv)+i;

			break; 	
		}
		i += 2; //try to read the next option header
	}
}

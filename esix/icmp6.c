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
void esix_icmp_process_packet(struct icmp6_hdr *icmp_hdr, int length, struct ip6_hdr *ip_hdr )
{
	//check if we have enough bytes to read the ICMP header
	if(length < 4)
		return;
uart_printf("ICMP type: %x addr: %x %x %x %x\n",icmp_hdr->type,ip_hdr->daddr1,ip_hdr->daddr2,ip_hdr->daddr3,ip_hdr->daddr4);
	//determine what to do next
	switch(icmp_hdr->type)
	{
		case NBR_SOL:
			///esix_icmp_process_neighbor_sol(
			//	(struct icmp6_rtr_adv *) &icmp_hdr->data, length - 4, ip_hdr);
			break;
		case RTR_ADV:
			toggle_led();
			esix_icmp_process_router_adv(
				(struct icmp6_router_adv *) &icmp_hdr->data, length - 4, ip_hdr);
			break;
		case ECHO_RQ: 
			esix_icmp_process_echo_req(
				(struct icmp6_echo_req *) &icmp_hdr->data, length - 4, ip_hdr);
			break;
		default:	
			return;
	}
}


/**
 * Sends a TTL expired message back to its source.
 */
void	esix_icmp_send_ttl_expired(struct ip6_hdr *hdr)
{
}

/**
 * Crafts and sends a router sollicitation
 * on the interface specified by index. 
 */
void	esix_icmp_send_router_sol(int intf_index)
{
}

/**
 * Parses RA messages, add/update a default route,
 * a prefix route and builds an IP address out of this prefix.
 */
void esix_icmp_process_router_adv(struct icmp6_router_adv *rtr_adv, int length,
	 struct ip6_hdr *ip_hdr)
{
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
				mtu_info = (struct icmp6_opt_mtu *) &option_hdr->payload; 
				if( (i+8) < ntoh16(length) )
				{
					mtu	= ntoh16(mtu_info->mtu);
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
		esix_intf_add_route(	0x0, 0x0, 0x0, 0x0, 	//default dest
					0x0,			//default mask
					ip_hdr->saddr1,		//next hop
					ip_hdr->saddr2,
					ip_hdr->saddr3,
					ip_hdr->saddr4,
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
		esix_intf_add_address(  	hton32(pfx_info->p[0] << 24	//prefix 
					 | pfx_info->p[1] << 16 
					 | pfx_info->p[2] << 8
					 | pfx_info->p[3]),
					hton32(pfx_info->p[4] << 24 	//prefix
					 | pfx_info->p[5] << 16 
					 | pfx_info->p[6] << 8
					 | pfx_info->p[7]), // FIXME: mac adress in the table ?
					hton32((neighbors[0]->mac_addr.l | 0x020000ff)),	// 0x02 : universal bit
					hton32((0xfe000000) | ((neighbors[0]->mac_addr.l << 16) & 0xff0000) | neighbors[0]->mac_addr.h),
				0x40,				// /64
				esix_w_get_time() + pfx_info->valid_lifetime, //expiration date
				GLOBAL_SCOPE);

		//onlink route (local route for our own subnet)
		esix_intf_add_route(  	hton32(pfx_info->p[0] << 24	//prefix 
					 | pfx_info->p[1] << 16 
					 | pfx_info->p[2] << 8
					 | pfx_info->p[3]),
					hton32(pfx_info->p[4] << 24 	//prefix
					 | pfx_info->p[5] << 16 
					 | pfx_info->p[6] << 8
					 | pfx_info->p[7]),
					0x0,
					0x0,
					0x40,				//ALWAYS /64 for autoconf.
					0x0,		
					0x0,				//No next hop means on-link.
					0x0,
					0x0,
					esix_w_get_time() + ntoh32(pfx_info->valid_lifetime), //exp. date
					rtr_adv->cur_hlim,	//TTL
					mtu,
					INTERFACE);
	}//else if (got_prefix_info)
}

void esix_icmp_process_echo_req(struct icmp6_echo_req *echo_rq, int length, struct ip6_hdr *ip_hdr)
{
	uart_printf("ethernet stack %x\n", uxTaskGetStackHighWaterMark(NULL));
	uart_puts("echo rq received\n");
	
}


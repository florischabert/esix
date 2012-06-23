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

	// check the checksum
	if(esix_ip_upper_checksum(&ip_hdr->saddr, &ip_hdr->daddr, ICMP, icmp_hdr, length) != 0)
		return;
		
	//determine what to do next
	switch(icmp_hdr->type)
	{
		case NBR_SOL:
			//these are just sanity checks : only code 0 are currently defined.
			if(icmp_hdr->code == 0)
				esix_icmp_process_neighbor_sol(
					(struct icmp6_neighbor_sol *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		case NBR_ADV:
			if(icmp_hdr->code == 0)
				esix_icmp_process_neighbor_adv(
					(struct icmp6_neighbor_adv *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		case RTR_ADV:
			if(icmp_hdr->code == 0)
				esix_icmp_process_router_adv(
					(struct icmp6_router_adv *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		case ECHO_RQ: 
			esix_icmp_process_echo_req(
				(struct icmp6_echo *) (icmp_hdr + 1), length - 4, ip_hdr);
			break;
		case MLD_QRY:
			esix_icmp_process_mld_query(
 				(struct icmp6_mld1_hdr *) (icmp_hdr + 1), length - 4, ip_hdr);
			//esix_icmp_send_mld2_report();
			break;
		default:
			;
	}
}

/*
 * Send an ICMPv6 packet
 */
void esix_icmp_send(const struct ip6_addr *_saddr, const struct ip6_addr *daddr, uint8_t hlimit, uint8_t type, uint8_t code, void *data, uint16_t len)
{
	struct icmp6_hdr *hdr;
	struct ip6_addr saddr = *_saddr;

	if((hdr = malloc(sizeof(struct icmp6_hdr) + len)) == NULL)
	{
		free(data);	
		return;
	}

	hdr->type = type;
	hdr->code = code;
	hdr->chksum = 0;
	esix_memcpy(hdr + 1, data, len);
	
	free(data);	
	
	//check the source address. If it's multicast, replace it.
	//If we can't replace it (no adress available, which should never happen),
	//abort and destroy the packet.
	if(esix_intf_check_source_addr(&saddr, daddr) < 0)
	{
		free(hdr);
		return;
	}
	
	hdr->chksum = esix_ip_upper_checksum(&saddr, daddr, ICMP, hdr, len + sizeof(struct icmp6_hdr));
	
	esix_ip_send(&saddr, daddr, hlimit, ICMP, hdr, len + sizeof(struct icmp6_hdr));

	free(hdr);
	
}

/**
 * Sends a TTL expired message back to its source.
 */
void esix_icmp_send_ttl_expired(const struct ip6_hdr *ip_hdr)
{
	//TODO : don't reply to an icmp error message

	//we're returning (most of) an entire packet. The size of our error msg shouldn't
	//ever exceed the minimum IPv6 MTU (1280 bytes).
	int n_len = ntoh16(ip_hdr->payload_len) + sizeof(struct ip6_hdr) + sizeof(struct icmp6_ttl_exp_hdr);
	if(n_len > 1280 - sizeof(struct ip6_hdr) - sizeof(struct icmp6_hdr))
		n_len=1280 - sizeof(struct ip6_hdr) - sizeof(struct icmp6_hdr);

	struct icmp6_ttl_exp_hdr *ttl_exp = malloc(n_len);
	//hmm, I smell gas...
	if(ttl_exp == NULL)
		return;

	//now copy the packet that caused trouble.
	esix_memcpy(ttl_exp+1, ip_hdr, n_len-sizeof(struct icmp6_ttl_exp_hdr));

	esix_icmp_send(&ip_hdr->daddr, &ip_hdr->saddr, 255, TTL_EXP , 0, ttl_exp, n_len);
}

/**
 * Sends an ICMP unreachable message back to its source.
 */
void esix_icmp_send_unreachable(const struct ip6_hdr *ip_hdr, uint8_t type)
{
	//TODO : don't reply to an icmp error message

	//we're returning (most of) an entire packet. The size of our error msg shouldn't
	//ever exceed the minimum IPv6 MTU (1280 bytes).
	int n_len = ntoh16(ip_hdr->payload_len) + sizeof(struct ip6_hdr) + sizeof(struct icmp6_unreachable_hdr);
	if(n_len > 1280 - sizeof(struct ip6_hdr) - sizeof(struct icmp6_hdr))
		n_len=1280 - sizeof(struct ip6_hdr) - sizeof(struct icmp6_hdr);

	struct icmp6_unreachable_hdr *unreach = malloc(n_len);
	//hmm, I smell gas...
	if(unreach == NULL)
		return;

	//now copy the packet that caused trouble.
	esix_memcpy(unreach+1, ip_hdr, n_len - sizeof(struct icmp6_unreachable_hdr));

	esix_icmp_send(&ip_hdr->daddr, &ip_hdr->saddr, 255, DST_UNR, type, unreach, n_len);
}

/**
 * Crafts and sends a router sollicitation
 * on the interface specified by index. 
 */
void esix_icmp_send_router_sol(uint8_t intf_index)
{
	int i;
	struct ip6_addr dest;
	dest.addr1	= hton32(0xff020000);
	dest.addr2	= hton32(0x00000000);
	dest.addr3	= hton32(0x00000000);
	dest.addr4	= hton32(0x00000002);
	
	uint16_t len = sizeof(struct icmp6_router_sol) + sizeof(struct icmp6_opt_lla);
	struct icmp6_router_sol *ra_sol = malloc(len);

	//hmm, I smell gas...
	if(ra_sol == NULL)
		return;

	struct icmp6_opt_lla *opt = (struct icmp6_opt_lla *) (ra_sol + 1);

	opt->type	= S_LLA;
	opt->len8	= 1; //1 * 8 bytes
	for(i=0; i<3; i++)
		opt->lla[i]	= neighbors[0]->lla[i];

	if((i=esix_intf_get_type_address(LINK_LOCAL)) >=  0)
		esix_icmp_send(&addrs[i]->addr, &dest, 255, RTR_SOL, 0, ra_sol, len);
}

/*
 * Process a neighbor solicitation
 */
void esix_icmp_process_neighbor_sol(struct icmp6_neighbor_sol *nb_sol, int len, struct ip6_hdr *hdr)
{
	int i;

	//sanity checks
	//- ICMP length (derived from the IP length) is 24 or more octets (24 = 8 ICMP bytes + 16 NS bytes)
	//- hop limit should be 255
	//- target address is not a multicast address
	// TODO: others non implemented yet.
	if( 	(len < 16) || 
		hdr->hlimit != 255 ||
		(nb_sol->target_addr.addr1 & hton32(0xff000000)) == hton32(0xff000000))
		return;

	//check that the sollicitation is actually for us
	if(esix_intf_get_address_index(&nb_sol->target_addr, ANY, ANY_MASK) < 0)
		return;

	i = esix_intf_get_neighbor_index(&hdr->saddr, INTERFACE);
	if(i < 0) // the neighbor isn't in the cache, add it
	{
		esix_intf_add_neighbor(&hdr->saddr, ((struct icmp6_opt_lla *) (nb_sol + 1))->lla, 
			esix_get_time() + NEW_NEIGHBOR_TIMEOUT, INTERFACE);
		//try again, to set some flags.
		//note that even if it wasn't added (e.g. due to a full table),
		//we still need to send an advertisement.
		if((i = esix_intf_get_neighbor_index(&hdr->saddr, INTERFACE)) >= 0)
		{
			neighbors[i]->flags.sollicited	= ND_UNSOLLICITED;
			neighbors[i]->flags.status	= ND_STALE;
		}
	}
		
	//actually send the advertisement
	esix_icmp_send_neighbor_adv(&nb_sol->target_addr, &hdr->saddr, 1);
}

/*
 * Process a neighbor advertisement
 */
void esix_icmp_process_neighbor_adv(struct icmp6_neighbor_adv *nb_adv, int len, struct ip6_hdr *ip_hdr)
{
	int i;
	//sanity checks
	//- ICMP length (derived from the IP length) is 24 or more octets (24 = 8 ICMP bytes + 16 NS bytes)
	//- hop limit should be 255
	//- target address is not a multicast address
	// TODO: others non implemented yet.
	if( 	(len < 16) || 
		ip_hdr->hlimit != 255 ||
		(nb_adv->target_addr.addr1 & hton32(0xff000000)) == hton32(0xff000000))
		return;

	i = esix_intf_get_neighbor_index(&nb_adv->target_addr, INTERFACE);

	if(i < 0) // the neighbor isn't in the cache, add it
	{
		esix_intf_add_neighbor(&nb_adv->target_addr, 
			((struct icmp6_opt_lla *) (nb_adv + 1))->lla, 
			esix_get_time() + NEIGHBOR_TIMEOUT, INTERFACE);
		//now find it again to set some flags
		i = esix_intf_get_neighbor_index(&nb_adv->target_addr, INTERFACE);
		if(i<0)
			return;
	}
	else
	{
		//check that we actually asked for this advertisement
		//(aka make sure nobody is messing with our cache)
		if(neighbors[i]->flags.sollicited != ND_SOLLICITED)
			return;
	}

	neighbors[i]->flags.sollicited	= ND_UNSOLLICITED;
	neighbors[i]->flags.status	= ND_REACHABLE;
	neighbors[i]->expiration_date	= esix_get_time() + NEIGHBOR_TIMEOUT;
}

/*
 * Process an ICMPv6 Echo Request and send an echo reply.
 */
void esix_icmp_process_echo_req(struct icmp6_echo *echo_req, int len, struct ip6_hdr *ip_hdr)
{
	struct icmp6_echo *echo_rep = malloc(len);
	if(echo_rep == NULL)
		return;
	//copying the whole packet and sending it back to its source should do the trick.	
	esix_memcpy(echo_rep, echo_req, len);

	esix_icmp_send(&ip_hdr->daddr, &ip_hdr->saddr, 64, ECHO_RP, 0, echo_rep, len);
}

/*
 * Send a neighbor advertisement.
 */
void esix_icmp_send_neighbor_adv(const struct ip6_addr *saddr, const struct ip6_addr *daddr, int is_solicited)
{
	uint16_t len = sizeof(struct icmp6_neighbor_adv) + sizeof(struct icmp6_opt_lla);

	struct icmp6_neighbor_adv *nb_adv = malloc(len);
	if(nb_adv == NULL)
		return;

	struct icmp6_opt_lla *opt = (struct icmp6_opt_lla *) (nb_adv + 1);
	
	nb_adv->r_s_o_reserved = hton32(is_solicited << 30);
	nb_adv->target_addr = *saddr;
	
	opt->type = 2; // Target Link-Layer Address
	opt->len8 = 1; // length: 1x8 bytes
	opt->lla[0] = neighbors[0]->lla[0];
	opt->lla[1] = neighbors[0]->lla[1];
	opt->lla[2] = neighbors[0]->lla[2];

	esix_icmp_send(saddr, daddr, 255, NBR_ADV, 0, nb_adv, len);
}

/*
 * Send a neighbor sollicitation.
 */
void esix_icmp_send_neighbor_sol(const struct ip6_addr *saddr, const struct ip6_addr *daddr)
{
	//don't add a l2 address if we don't have one yet (which can only happen
	//when performing first DAD)
	uint16_t len;
	if(neighbors[0] != NULL)
		len = sizeof(struct icmp6_neighbor_sol) + sizeof(struct icmp6_opt_lla);
	else
		len = sizeof(struct icmp6_neighbor_sol);

	struct icmp6_neighbor_sol *nb_sol = malloc(len);
	if(nb_sol == NULL)
		return;
	struct icmp6_opt_lla *opt = (struct icmp6_opt_lla *) (nb_sol + 1);
	struct ip6_addr	mcast_dst;
	int i=0;
	
	nb_sol->target_addr = *daddr;
	nb_sol->reserved = 0;

	//build the ipv6 multicast dest address
	mcast_dst.addr1	= hton32(0xff020000);
	mcast_dst.addr2	= hton32(0x00000000);
	mcast_dst.addr3	= hton32(0x00000001);
	mcast_dst.addr4	= hton32(0xff << 24 | (ntoh32(daddr->addr4) & 0x00ffffff));
	
	if(neighbors[0] != NULL)
	{
		opt->type = 1; // Source Link-Layer Address
		opt->len8 = 1; // length: 1x8 bytes
		opt->lla[0] = neighbors[0]->lla[0];
		opt->lla[1] = neighbors[0]->lla[1];
		opt->lla[2] = neighbors[0]->lla[2];
	}

	//do we know it already? then send an unicast sollicitation
	if( (i=esix_intf_get_neighbor_index(daddr, INTERFACE)) >= 0)
	{
		neighbors[i]->flags.sollicited	= ND_SOLLICITED;
		neighbors[i]->flags.status	= ND_STALE;
		esix_icmp_send(saddr, daddr, 255, NBR_SOL, 0, nb_sol, len);
	}
	else 
		esix_icmp_send(saddr, &mcast_dst, 255, NBR_SOL, 0, nb_sol, len);
}

/**
 * Parses RA messages, add/update a default route,
 * a prefix route and builds an IP address out of this prefix.
 */
void esix_icmp_process_router_adv(struct icmp6_router_adv *rtr_adv, int length,
	 struct ip6_hdr *ip_hdr)
{
	struct ip6_addr addr, addr2, mask;
	int i=0;
	uint32_t mtu;
	struct icmp6_opt_prefix_info *pfx_info = NULL;
	struct icmp6_opt_mtu *mtu_info;
	struct icmp6_option_hdr *option_hdr;
	int got_prefix_info	= 0;

	//Sanity checks

	//- we at least need to have 16 bytes to parse 
	//- the hop limit field should be set to 255.
	//(router advertisement without any option = 16 bytes,
	//advertises only a default route)
	//- source address scope is link local
	if(ntoh16(length) < 16 || ip_hdr->hlimit != 255 ||
		ip_hdr->saddr.addr1 != hton32(0xfe800000) ||
		ip_hdr->saddr.addr2 != hton32(0x00000000))
		return;

	mtu	= DEFAULT_MTU;
				
	//parse options like MTU and prefix info
	i=12; 	//we at least need 2 more bytes (type + length) to be able to process
			//the first option field (those are TLVs)
	while (i + 2 < length) // is the ip packet long enough to continue?
	{
		option_hdr = (struct icmp6_option_hdr *) ((uint8_t *) rtr_adv + i);
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
					mtu	= ntoh32(mtu_info->mtu);
				}
				i+=8;
			break;

			default:
				//this is an easy way to make the stack hang...
				//(and should also be considered as a malformed packet. easy cure : drop it.)
				if(option_hdr->length == 0)
				{
					return;
				}
				else
					i+= (option_hdr->length*8); //option_hdr->length gives the size of
								//type + length + data fields in
								//8 bytes multiples
								//skip this length since we don't know
								//how to process it
			break; 	
		}
	}

	//now add the routes (we needed the MTU value first)

	//global address && onlink route
	if (got_prefix_info)
	{
	
		//builds a new global scope address
		//not endian-safe for now, words in the prefix field
		//are not aligned when received
		addr.addr1 = 	hton32(	pfx_info->p[0] << 24
					| pfx_info->p[1] << 16
					| pfx_info->p[2] << 8
					| pfx_info->p[3]);

		addr.addr2 = 	hton32(	pfx_info->p[4] << 24
					| pfx_info->p[5] << 16
					| pfx_info->p[6] << 8
					| pfx_info->p[7]);

		addr.addr3 = 	hton32(	(ntoh16(neighbors[0]->lla[0]) << 16 & 0xff0000)
					| (ntoh16(neighbors[0]->lla[1]) & 0xff00)
					| (0x020000ff) ); //stateless autoconf, 0x02 : universal bit

		addr.addr4 = 	hton32(	(0xfe000000) //0xfe here is OK
			 		| (ntoh16(neighbors[0]->lla[1]) << 16 & 0xff0000) 
			 		| (ntoh16(neighbors[0]->lla[2])) );

		addr2.addr1 = 0;
		addr2.addr2 = 0;
		addr2.addr3 = 0;
		addr2.addr4 = 0;
		mask.addr1  = 0xffffffff;
		mask.addr2  = 0xffffffff;
		mask.addr3  = 0;
		mask.addr4  = 0;


		//a lifetime of 0 means remove the address/route
		if(pfx_info->valid_lifetime == 0x0)
		{
			//remove the prefix-associated address and route
			esix_intf_remove_address(&addr, 0x40, GLOBAL);
			addr.addr3 = 0;
			addr.addr4 = 0;
			esix_intf_remove_route(&addr, &mask, &addr2, INTERFACE);
		}
		else
		{
			esix_intf_add_address(&addr,
					0x40,				// /64
					esix_get_time() + ntoh32(pfx_info->valid_lifetime), //expiration date
					GLOBAL);
			addr.addr3 = 0;
			addr.addr4 = 0;

			//onlink route (local route for our own subnet)
			esix_intf_add_route(&addr, &mask, &addr2,
					esix_get_time() + ntoh32(pfx_info->valid_lifetime), //exp. date
					rtr_adv->cur_hlim,	//TTL
					mtu,
					INTERFACE);
		}

	}//else if (got_prefix_info)

	//default route
	//a lifetime of 0 means remove the route
	addr.addr1 = 0;
	addr.addr2 = 0;
	addr.addr3 = 0;
	addr.addr4 = 0;
	mask.addr1 = 0;
	mask.addr2 = 0;
	mask.addr3 = 0;
	mask.addr4 = 0;
	
	if(ntoh16(rtr_adv->rtr_lifetime) == 0x0)
		esix_intf_remove_route(&addr, &mask, &ip_hdr->saddr, INTERFACE);
	else
	
		esix_intf_add_route(&addr, 	//default dest
					&mask,			//default mask
					&ip_hdr->saddr,		//next hop
					esix_get_time() + ntoh16(rtr_adv->rtr_lifetime), //exp. date
					rtr_adv->cur_hlim,	//TTL
					mtu,
					INTERFACE);

}

//this is buggy and hasn't been updated. short disclamer : don't user it.
void esix_icmp_send_mld2_report(void)
{
	int i=0;
	uint16_t count=0;
	int len;
	int index_list[ESIX_MAX_IPADDR];

	struct ip6_addr all_mld2_queriers;
	all_mld2_queriers.addr1	= hton32(0xff020000);
	all_mld2_queriers.addr2	= hton32(0x00000000);
	all_mld2_queriers.addr3	= hton32(0x00000000);
	all_mld2_queriers.addr4	= hton32(0x00000016);

	while(i < ESIX_MAX_IPADDR)
	{	
		index_list[i] = 0;
		if(addrs[i] != NULL && 
			(addrs[i]->addr.addr1 & hton32(0xff000000)) == hton32(0xff000000))
		{
			index_list[i] = 1;
			count++;
		}
		i++;
	}

	len	= sizeof(struct icmp6_mld2_hdr) + sizeof(struct icmp6_mld2_opt_mcast_addr_record)*count;
	struct icmp6_mld2_hdr *mld = malloc(len);

	//hm, I smell gas...
	if(mld == NULL)
		return;
	
	mld->num_mcast_addr_records	= hton16(count);
	struct icmp6_mld2_opt_mcast_addr_record *mld_mcast; 

	i=0;
	count=0;
	mld_mcast = (struct icmp6_mld2_opt_mcast_addr_record*) (mld+1);
	//TODO : check that we don't exceed the MTU (in which case we should send 2 separate reports)
	while(i < ESIX_MAX_IPADDR)
	{
		//for each multicast address previously found, append a record in our report
		if(index_list[i] == 1)
		{
			mld_mcast = mld_mcast + count;
			mld_mcast->record_type  = MLD2_CHANGE_TO_INCLUDE; //TODO: we never change to exclude...
			mld_mcast->aux_data_len = 0; //must be 0 per RFC
			mld_mcast->num_sources  = 0; //we're far from supporting SSM yet...
			mld_mcast->addr         = addrs[i]->addr;
			count++;
		}
		i++;
	}

	esix_icmp_send(&addrs[0]->addr, &all_mld2_queriers, 1, MLD2_RP, 0, mld, len);
}



void esix_icmp_process_mld_query(struct icmp6_mld1_hdr *mld, int len, struct ip6_hdr *ip_hdr)
{
        int i=0;
	int a = esix_intf_get_type_address(LINK_LOCAL); 
        //make sure we got enough data to process our packet
	//and a valid link local address to send our reply
        if(len < sizeof(struct icmp6_mld1_hdr) || a < 0)
                return;

        //this is a general query
        if(len == sizeof(struct icmp6_mld1_hdr))
        {
                while(i < ESIX_MAX_IPADDR)
                {
                        if(addrs[i] != NULL &&
                                (addrs[i]->addr.addr1 & hton32(0xff000000)) == hton32(0xff000000))
                        {
                                esix_icmp_send_mld(&addrs[a]->addr, MLD_RPT);
                        }
                        i++;
                }
        }
        //specific query
        else if((len ==  sizeof(struct icmp6_mld1_hdr) +  sizeof(struct ip6_hdr))
                && ((i = esix_intf_get_address_index((struct ip6_addr*) (mld+1), MULTICAST, ANY_MASK)) > 0))
        {
                esix_icmp_send_mld(&addrs[a]->addr, MLD_RPT);
        }
}

void esix_icmp_send_mld(const struct ip6_addr *mcast_addr, int mld_type)
{
        //don't send any report for the all-nodes address
        if( (mcast_addr->addr1 == hton32(0xff020000)) &&
                (mcast_addr->addr2 == hton32(0x00000000)) &&
                (mcast_addr->addr3 == hton32(0x00000000)) &&
                (mcast_addr->addr4 == hton32(0x00000001)) )
                return;

        //TODO: implement timers
        struct icmp6_mld1_hdr *hdr;
        struct ip6_addr *target = (struct ip6_addr*) (hdr+1);
	int i = esix_intf_get_type_address(LINK_LOCAL); 


	if(i < 0)
		return;

        if((hdr = malloc(sizeof(struct icmp6_mld1_hdr) + sizeof(struct ip6_addr))) == NULL)
                return;

        hdr->max_resp_delay = 0;
        *target = *mcast_addr;

        if(mld_type == MLD_RPT)
        {
                esix_icmp_send(&addrs[i]->addr, mcast_addr, 1, MLD_RPT, 0, hdr,
                        sizeof(struct icmp6_mld1_hdr) + sizeof(struct ip6_addr));
        }
        else //must be a 'done'
        {
                struct ip6_addr all_nodes;
                all_nodes.addr1 = hton32(0xff020000);
                all_nodes.addr2 = hton32(0x00000000);
                all_nodes.addr3 = hton32(0x00000000);
                all_nodes.addr4 = hton32(0x00000001);

                esix_icmp_send(&addrs[i]->addr, &all_nodes, 1, MLD_DNE, 0, hdr,
                        sizeof(struct icmp6_mld1_hdr) + sizeof(struct ip6_addr));
        }
}


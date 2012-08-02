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

#ifndef _ICMP6_H
#define _ICMP6_H
 
#include "tools.h"
#include "config.h"
#include "ip6.h"
#include "intf.h"
#include "tcp6.h"
#include "udp6.h"
#include <esix.h>

//neighbor discovery settings
#define NEW_NEIGHBOR_TIMEOUT 	5
#define NEIGHBOR_TIMEOUT	180	
#define STALE_DURATION 		3
#define DUP_ADDR_DETECT_TRANSMITS 1

//list of ICMPv6 types
#define DST_UNR 0x01    //Destination Unreachable
#define TOO_BIG 0x02    //Packet Too Big
#define TTL_EXP 0x03    //TTL Exceeded
#define PARAM_P 0x04    //Parameter problem
#define ECHO_RQ 0x80    //Echo request
#define ECHO_RP 0x81    //Echo reply
#define MLD_QRY 0x82    //Multicast listener query
#define MLD_RPT 0x83    //Multicast listener report
#define MLD_DNE 0x84    //Multicast listener done
#define RTR_SOL 0x85    //Router sollicitation
#define RTR_ADV 0x86    //Router advertisement
#define NBR_SOL 0x87    //Neighbor sollicitation
#define NBR_ADV 0x88    //Neighbor advertisement
#define REDIR   0x89    //Redirect
#define MLD2_RP 0x8f    //Multicast Listener Report (MLDv2)

//list of ICMPv6 options
#define PRFX_INFO 	0x3
#define MTU		0x5
#define S_LLA		0x1

//various icmp constants
#define PORT_UNREACHABLE 4
#define ADDR_UNREACHABLE 3
#define ADMIN_PROHIBITED 1
#define NO_ROUTE_TO_DEST 0

//mldv2 constants
#define MLD2_CHANGE_TO_INCLUDE 3  
#define MLD2_CHANGE_TO_EXCLUDE 4  

/**
 * ICMPv6 header
 */
struct icmp6_hdr {
	uint8_t	type;
	uint8_t	code;
	uint16_t	chksum;
	// ICMPv6 data
} __attribute__((__packed__));

/**
 * ICMP option header
 */
struct icmp6_option_hdr
{
	uint8_t	type;
	uint8_t	length;
	uint8_t	payload;
} __attribute__((__packed__));

/**
 * ICMP neighbor solicitation header.
 */	
struct icmp6_neighbor_sol
{
	uint32_t reserved;
	esix_ip6_addr target_addr;
} __attribute__((__packed__));

/**
 * ICMP neighbor advertisement header.
 */	
struct icmp6_neighbor_adv
{
	uint32_t r_s_o_reserved;
	esix_ip6_addr target_addr;
} __attribute__((__packed__));

/**
 * ICMP option, link-layer address.
 */
struct icmp6_opt_lla {
	uint8_t type;
	uint8_t len8;
	esix_eth_addr lla;
} __attribute__((__packed__));

/**
 * ICMP Router Sollicitation header.
 */
struct icmp6_router_sol {
	uint32_t reserved;
} __attribute__((__packed__));

/**
 * ICMP Router Advertisement header.
 */
struct icmp6_router_adv {
	uint8_t	cur_hlim;	//hlim to be used 
	uint8_t	flags;		//other config, router preference, etc...
	uint16_t	rtr_lifetime;	//router lifetime
	uint32_t	reachable_time;
	uint32_t	retransm_timer;
} __attribute__((__packed__));

/**
 * ICMP Echo Request/Reply header.
 */
struct icmp6_echo {
	uint16_t	identifier;
	uint16_t	seq_num;
} __attribute__((__packed__));

/**
 * ICMP TTL expired header.
 */
struct icmp6_ttl_exp_hdr {
	uint32_t reserved; //do we really need such a header? hm...
} __attribute__((__packed__));

/**
 * ICMP unreachable header.
 */
struct icmp6_unreachable_hdr {
	uint32_t reserved; //do we really need such a header? hm...
} __attribute__((__packed__));

/**
 * ICMP option, prefix info header.
 */
struct icmp6_opt_prefix_info {
	uint8_t	prefix_length;
	uint8_t	flags;		//onlink, autoconf,...
	uint32_t	valid_lifetime;
	uint32_t	preferred_lifetime;
	uint32_t	reserved;
	uint8_t p[16];
} __attribute__((__packed__));

/**
 * ICMP option, MTU.
 */
struct icmp6_opt_mtu {
	uint16_t reserved;
	uint32_t	mtu;
} __attribute__((__packed__));

/**
 * MLDv1 header
 */
struct icmp6_mld1_hdr {
        uint16_t max_resp_delay;
        uint16_t reserved;
}  __attribute__((__packed__));

/**
 * MDLv2 header
 */
struct icmp6_mld2_hdr {
	uint16_t reserved;
	uint16_t num_mcast_addr_records;
} __attribute__((__packed__));

/**
 * MLDv2 option, mcast address record header
 */
struct icmp6_mld2_opt_mcast_addr_record {
	uint8_t record_type;
	uint8_t aux_data_len;
	uint16_t num_sources;
	esix_ip6_addr addr;
} __attribute__((__packed__));

/**
 * MLDv2 option, source address specifier (for SSM)
 */
struct mld2_opt_source_addr {
	esix_ip6_addr addr;
} __attribute__((__packed__));

void esix_icmp6_process(const void *payload, int length, const esix_ip6_hdr *ip_hdr );
void esix_icmp6_send(const esix_ip6_addr *saddr, const esix_ip6_addr *daddr, uint8_t hlimit, uint8_t type, uint8_t code, void *data, uint16_t len);

void esix_icmp6_send_ttl_expired(const esix_ip6_hdr *hdr);
void esix_icmp6_send_router_sol(uint8_t intf_index);
void esix_icmp6_send_neighbor_adv(const esix_ip6_addr *, const esix_ip6_addr *, int);
void esix_icmp6_process_neighbor_sol(struct icmp6_neighbor_sol *nb_sol, int len, const esix_ip6_hdr *hdr);
void esix_icmp6_process_router_adv(struct icmp6_router_adv *rtr_adv, int length, const esix_ip6_hdr *ip_hdr);
void esix_icmp6_process_echo_req(struct icmp6_echo *echo_rq, int length, const esix_ip6_hdr *ip_hdr);
void esix_icmp6_process_neighbor_adv(struct icmp6_neighbor_adv *, int , const esix_ip6_hdr *);
void esix_icmp6_send_neighbor_sol(const esix_ip6_addr*, const esix_ip6_addr*);
void esix_icmp6_send_router_sol(uint8_t);
void esix_icmp6_send_unreachable(const esix_ip6_hdr *ip_hdr, uint8_t type);
void esix_icmp6_process_mld_query(struct icmp6_mld1_hdr *, int, const esix_ip6_hdr *);
void esix_icmp6_send_mld(const esix_ip6_addr *, int);
void esix_icmp6_send_mld2_report(void);

#endif

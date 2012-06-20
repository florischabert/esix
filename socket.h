#ifndef _SOCKET_H
#define _SOCKET_H

#include "esix.h"
#include "include/socket.h"
#include "ip6.h"

enum state
{
	CLOSED,
	LISTEN,
	SYN_SENT,
	SYN_RECEIVED,
	ESTABLISHED,
	FIN_WAIT_1,
	FIN_WAIT_2,
	CLOSE_WAIT,
	CLOSING,
	LAST_ACK,
	TIME_WAIT,
	RESERVED //internal state
};

enum direction
{
	IN,
	OUT
};

enum action
{
	KEEP,
	EVICT
};

//queue element type
enum qe_type
{
	CHILD_SOCK, //child socket, created upon SYN reception
	SENT_PKT,
	RECV_PKT
};

struct sock_queue
{
	enum qe_type qe_type;
	int socknum; 			//only used with CHILD_SOCK
	struct sockaddr_in6 *sockaddr;  //only used by UDP for RX packets, addr&port of sender
	u32_t seqn;			//stores the sequence number of this packet
	void *data; //actual data
	int data_len; //data length
	u32_t t_sent; //time at which the packet was queued
	struct sock_queue *next_e; //next queued element
};

//actual session sockets, holding seq/ack/etc info
struct esix_sock
{
	u8_t proto;
	volatile enum state state;
	struct ip6_addr laddr;
	struct ip6_addr raddr;
	u16_t lport;
	u16_t rport;
	u32_t seqn;
	u32_t ackn;
	u32_t rexmit_date; //date at which to trigger retransmission
	struct sock_queue *queue; //stores sent/recvd data
};

#define FIND_ANY 0
#define FIND_CONNECTED 1
#define FIND_LISTEN 2

struct esix_sock esix_sockets[ESIX_MAX_SOCK];
u16_t esix_last_port;

int esix_port_available(const u16_t);
int esix_socket_create_child(const struct ip6_addr *, const struct ip6_addr *, u16_t, u16_t, u8_t);
int esix_find_socket(const struct ip6_addr *, const struct ip6_addr *, u16_t, u16_t, u8_t, u8_t);
int esix_queue_data(int, const void *, int, struct sockaddr_in6 *, enum direction);
struct sock_queue * esix_socket_find_e(int , enum qe_type, enum action);
void esix_socket_init();
void esix_socket_free_queue(int);
int esix_socket_expire_e(int, u32_t);
void esix_socket_housekeep();
#endif

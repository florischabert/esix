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
	int socknum; //only used with CHILD_SOCK
	struct sockaddr_in6 *sockaddr; //only used by UDP
	void *data; //actual data
	int data_len; //data length
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
int esix_queue_data(int, const void *, int, struct sockaddr_in6 *);
struct sock_queue * esix_socket_find_and_unlink(int , enum qe_type);
void esix_socket_init();
void esix_socket_free_queue(int);
#endif

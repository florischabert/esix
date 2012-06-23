#include "tcp6.h"
#include "tools.h"
#include "icmp6.h"
#include "intf.h"
#include "include/socket.h"
#include "socket.h"


const struct in6_addr in6addr_any = {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}};
const struct in6_addr in6addr_loopback = {{{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0}}};

void esix_socket_init()
{
	int i=ESIX_MAX_SOCK;
	while(i-->0)
		esix_sockets[i].state = CLOSED;
}

int esix_queue_data(int sock, const void *data, int len, struct sockaddr_in6 *sockaddr, enum direction direction)
{
	//sock queue element 
	struct sock_queue *sqe, *cur_sqe;
	int i;
	uint8_t *buf;

	//don't queue up more than ESIX_QUEUE_DEPHT packets
	cur_sqe = esix_sockets[sock].queue; 
	for(i=0; cur_sqe != NULL ; i++)
	{
		if(i >= ESIX_QUEUE_DEPHT)
			return -1;

		cur_sqe = cur_sqe->next_e;
	}

	switch(esix_sockets[sock].proto)
	{
		case SOCK_DGRAM:
			if((buf = malloc(len+sizeof(struct sockaddr_in6))) == NULL ) 
				return -1;

			esix_memcpy(buf, sockaddr, sizeof(struct sockaddr_in6));
			esix_memcpy(buf+sizeof(struct sockaddr_in6), data, len);
		break;
		case SOCK_STREAM:
			if((buf = malloc(len)) == NULL ) 
				return -1;

			esix_memcpy(buf, data, len);
		break;
		default:
			return -1;
		break;
	}

	if((sqe = malloc(sizeof(struct sock_queue))) == NULL) 
	{
		free(buf);
		return -1;
	}

	sqe->data 	= buf;
	sqe->data_len 	= len;
	sqe->next_e	= NULL;

	if(direction == IN) 
		sqe->qe_type 	= RECV_PKT;
	else
	{
		sqe->qe_type	= SENT_PKT;
		sqe->seqn 	= esix_sockets[sock].seqn;
		sqe->t_sent 	= esix_get_time();
	}

	//there's no element in the list.
	if(esix_sockets[sock].queue == NULL)
		esix_sockets[sock].queue = sqe;
	else
	{
		//find the end of the list
		cur_sqe = esix_sockets[sock].queue;
		while(cur_sqe->next_e != NULL)
			cur_sqe = cur_sqe->next_e;

		cur_sqe->next_e	= sqe;
	}

	return len;
}

int sendto(int sock, const void *buf, int len, uint8_t flags, const struct sockaddr_in6 *to, int to_len)
{
	int i;
	//only to be used with UDP
	if(esix_sockets[sock].proto != SOCK_DGRAM)
		return -1;

	// check the source address
	if(esix_memcmp(&esix_sockets[sock].laddr, &in6addr_any, 16) == 0)
	{
		if(((i = esix_intf_get_type_address(GLOBAL)) <0) && 
			(i = esix_intf_get_type_address(LINK_LOCAL)) <0)
			return -1;

		esix_udp_send(&addrs[i]->addr, (struct ip6_addr*) &to->sin6_addr, 
			esix_sockets[sock].lport, to->sin6_port, buf, len);
	}
	else
		esix_udp_send(&esix_sockets[sock].laddr, (struct ip6_addr*) &to->sin6_addr, 
			esix_sockets[sock].lport, to->sin6_port, buf, len);

	return 0;
}

int connect(int sock, const struct sockaddr_in6 *daddr, int len)
{
	int i;
	if(esix_sockets[sock].proto == SOCK_STREAM)
	{
		if(esix_sockets[sock].state != RESERVED && 
			esix_sockets[sock].state != CLOSED)
			return -1;

		if((i=esix_intf_pick_source_address((struct ip6_addr*) daddr)) < 0)
			return -1;

		//connect() launches the tcp establishment procedure
		esix_sockets[sock].ackn = 0;
		esix_sockets[sock].rport = daddr->sin6_port;
		esix_memcpy(&esix_sockets[sock].raddr, &daddr->sin6_addr, 16);
		esix_sockets[sock].state = SYN_SENT;

		//send a SYN packet
		esix_tcp_send(&esix_sockets[sock].laddr, &esix_sockets[sock].raddr, 
			esix_sockets[sock].lport, esix_sockets[sock].rport, 
			esix_sockets[sock].seqn+1, esix_sockets[sock].ackn, SYN, NULL, 0);
	}
	else if(esix_sockets[sock].proto == SOCK_DGRAM)
	{
		 if(esix_sockets[sock].state != LISTEN &&
			esix_sockets[sock].state != ESTABLISHED)
			return -1;

		//this puts us in so called udp connected mode
		//only the source addr & port can talk to this socket
		esix_sockets[sock].rport = daddr->sin6_port;
		esix_memcpy(&esix_sockets[sock].raddr, &daddr->sin6_addr, 16);
		esix_sockets[sock].state = ESTABLISHED;
	}
	else return -1;

	return 0;
}

//finds (and unlink, if specified) the first element of type qe_type in a socket event queue 
//and returns a pointer.
struct sock_queue * esix_socket_find_e(int sock, enum qe_type qe_type, enum action action)
{
	struct sock_queue *sqe;
	struct sock_queue *prev_sqe;

	//empty queue
	if(esix_sockets[sock].queue == NULL)
		return NULL;

	sqe 		= esix_sockets[sock].queue;
	prev_sqe	= sqe;
	//find the first available eligible packet in queue
	//and keep a reference to the previous queue element
	while(sqe->qe_type != qe_type)
	{
		//we reached the end of the list
		if(sqe->next_e == NULL)
			return NULL;

		prev_sqe= sqe; 
		sqe 	= sqe->next_e;
	}

	//remove packet from queue
	if(action == EVICT)
	{
		//first element needs special treatment
		if(esix_sockets[sock].queue == sqe)
			esix_sockets[sock].queue = sqe->next_e;
		else
			prev_sqe->next_e = sqe->next_e;
	}

	return sqe;
}

int recv(int sock, void *buf, int max_len, uint8_t flags)
{
	return recvfrom(sock, buf, max_len, flags, NULL, NULL);
}

int recvfrom(int sock, void *buf, int max_len, int flags, struct sockaddr_in6 *sockaddr, int *sockaddr_len)
{
	//TODO : watch lockups due to OOM
	int len;

	if(esix_sockets[sock].proto == SOCK_STREAM && esix_sockets[sock].state != ESTABLISHED)
		return -1;

	struct sock_queue *sqe = esix_socket_find_e(sock, RECV_PKT, EVICT); 
	if(sqe == NULL)
		return 0;

	//TODO : hmm data loss warning here
	if(max_len < sqe->data_len)
		len = max_len;
	else
		len = sqe->data_len;

	switch(esix_sockets[sock].proto)
	{
		case SOCK_DGRAM:
			//copy the sockaddr_in6 struct
			if(sockaddr != NULL)
				esix_memcpy(sockaddr, sqe->data, sizeof(struct sockaddr_in6));
			//actual data
			esix_memcpy(buf, sqe->data+sizeof(struct sockaddr_in6), len);
		break;

		case SOCK_STREAM:
			//fill up the sockaddr_in6 struct with socket info
			//as TCP can only receive data in connected state
			if(sockaddr != NULL)
			{
				sockaddr->sin6_port = esix_sockets[sock].rport;
				esix_memcpy(&sockaddr->sin6_addr, &esix_sockets[sock].raddr, 16);
			}
			//actual data
			esix_memcpy(buf, sqe->data, len);
		break;
	}

	if(sockaddr_len != NULL)
		*sockaddr_len = sizeof(struct sockaddr_in6);

	//free data buffer and socket queue element
	free(sqe->data);
	free(sqe);

	return len;
}

int accept(int sock, struct sockaddr_in6 *saddr, int *sockaddr_len)
{
	int session_sock;
	struct sock_queue *sqe = esix_socket_find_e(sock, CHILD_SOCK, EVICT); 
	if(sqe == NULL)
		return -1;

	session_sock = sqe->socknum;
	free(sqe);

	if(saddr != NULL)
	{
		esix_memcpy(&saddr->sin6_addr, &esix_sockets[sock].raddr, 16);
		saddr->sin6_port = esix_sockets[sock].rport;
	}

	return session_sock;
}

//creates a session socket (actually only used by TCP when accept()'ing a client connection)
int esix_socket_create_child(const struct ip6_addr *saddr, const struct ip6_addr *daddr, uint16_t sport, uint16_t dport, uint8_t proto)
{
	struct sock_queue *sqe, *cur_sqe;
	//check if we're actually listening, if we are, create a new socket entry,
	//flag it as established and return its socket descriptor.
	int server_sock, session_sock;

	if((server_sock = esix_find_socket(saddr, daddr, sport, dport, proto, FIND_LISTEN)) < 0)
		return -1;

	//we found the server socket. try to create a service socket
	if((session_sock = socket(AF_INET6, proto, 0)) < 0)
		return -1;

	if((sqe = malloc(sizeof(struct sock_queue))) == NULL)
	{
		esix_sockets[session_sock].state = CLOSED;
		return -1;
	}

	//there's no element in the list.
	if(esix_sockets[server_sock].queue == NULL)
		esix_sockets[server_sock].queue = sqe;
	else
	{
		//find the end of the list
		cur_sqe = esix_sockets[server_sock].queue;
		while(cur_sqe->next_e != NULL)
			cur_sqe = cur_sqe->next_e;
		cur_sqe->next_e	= sqe;
	}

	sqe->qe_type = CHILD_SOCK;
	sqe->socknum = session_sock;
	sqe->next_e  = NULL;
	
	//copy remote addr stuff
	esix_memcpy(&esix_sockets[session_sock].raddr, saddr, 16);
	esix_sockets[session_sock].rport = sport;
	esix_sockets[session_sock].proto = proto;
	esix_memcpy(&esix_sockets[session_sock].laddr, daddr, 16);
	esix_sockets[session_sock].lport = dport;
	esix_sockets[session_sock].state = SYN_RECEIVED;

	return session_sock;
}

int esix_find_socket(const struct ip6_addr *saddr, const struct ip6_addr *daddr, uint16_t sport, uint16_t dport, uint8_t proto, uint8_t mask)
{
	int i=0;
	while(i<ESIX_MAX_SOCK)
	{
		//loop through the socket table until we find a match
		//first, look at protocol and port numbers.
		//then, let's see if either the packet was sent to the socket's 
		//local address or if the socket is listening on all interfaces
		if(esix_sockets[i].proto == proto && esix_sockets[i].lport == dport &&
			((esix_memcmp(daddr, &esix_sockets[i].laddr, 16) == 0) ||
			(esix_memcmp(&esix_sockets[i].laddr, &in6addr_any, 16) == 0)))
		{
			switch(mask)
			{
				case FIND_ANY:
					return i;
				break;

				case FIND_LISTEN:
					if(esix_sockets[i].state == LISTEN)
						return i;
				break;

				case FIND_CONNECTED:
					if(esix_sockets[i].state != CLOSED &&
						esix_sockets[i].state != RESERVED &&
						(esix_memcmp(&esix_sockets[i].raddr, saddr, 16) == 0) &&
						esix_sockets[i].rport == sport)
						return i;
				break;
			}
	
		}  
		i++;
	}
	//we couldn't find any socket capable of handling this packet.
	return -1;
}

int socket(const int family, const uint8_t type, const uint8_t proto)
{
	int i;
	//TODO : allow the same socket number on TCP & UDP

	//we only support inet6
	if(family != AF_INET6)
		return -1;

	//find a free port number
	while(1)
	{
		if(++esix_last_port > LAST_PORT || esix_last_port < FIRST_PORT)
			esix_last_port = FIRST_PORT;

		if(esix_port_available(esix_last_port) == 0)
			break;
	}

	//find a free socket number
	i=0;
	while(i<ESIX_MAX_SOCK)
	{
		//we found a free socket holder
		if(esix_sockets[i].state == CLOSED)
		{
			esix_sockets[i].state = RESERVED;
			esix_sockets[i].proto = type; //mmmm...
			esix_sockets[i].lport = esix_last_port;
			esix_sockets[i].rport = 0;
			esix_memcpy(&esix_sockets[i].laddr, &in6addr_any, 16);
			esix_memcpy(&esix_sockets[i].raddr, &in6addr_any, 16);
			esix_sockets[i].seqn = 0; //TODO : should be random
			esix_sockets[i].ackn = 0;
			esix_sockets[i].rexmit_date = 0;
			esix_sockets[i].queue = NULL;

			return i;
		}
		i++;
	}

	//the socket table is most likely full
	return -1;
}

int bind(const int socknum, const struct sockaddr_in6 *sockaddr, const int len)
{
	int i=0;
	if(len != sizeof(struct sockaddr_in6))
		return -1;

	//loop through the socket list to find if the port is available
	while(i<ESIX_MAX_SOCK)
	{
		if(esix_sockets[i].state != CLOSED &&
			esix_sockets[i].lport == sockaddr->sin6_port)
			return -1;
		i++;
	}


	//now check that we actually own the requested adress
	//if it's all zeroes, OK
	if(esix_memcmp(&in6addr_any, &sockaddr->sin6_addr, 16) == 0)
	{
		esix_sockets[socknum].lport = sockaddr->sin6_port;
		esix_memcpy(&esix_sockets[socknum].laddr, &in6addr_any, 16);
		return 0;
	}
	else //check that we own the address
	{
		i=0;
		while(i<ESIX_MAX_IPADDR)
		{
			if(addrs[i] != NULL &&
				esix_memcmp(&addrs[i]->addr, &sockaddr->sin6_addr, 16) == 0)
			{
				esix_sockets[socknum].lport = sockaddr->sin6_port;
				esix_memcpy(&addrs[i]->addr, &sockaddr->sin6_addr, 16);
				return 0;
			} 
		}
	}
	return -1;

}

int listen(int socket, int blacklog)
{
	if(esix_sockets[socket].state == RESERVED)
	{
		esix_sockets[socket].state = LISTEN;
		return 0;
	}
	return -1;
}

int close(const int socknum)
{
	if(esix_sockets[socknum].state == CLOSED || 
		esix_sockets[socknum].state == CLOSING)
		return -1;

	if(esix_sockets[socknum].proto == SOCK_STREAM)
	{
		switch(esix_sockets[socknum].state)
		{
			//FIXME : this is way too harsh
			case SYN_SENT:
			case SYN_RECEIVED:
			case CLOSING:
			case ESTABLISHED:
				esix_tcp_send(&esix_sockets[socknum].laddr, 
						&esix_sockets[socknum].raddr,
						esix_sockets[socknum].lport,
						esix_sockets[socknum].rport,
						esix_sockets[socknum].seqn,
						esix_sockets[socknum].ackn,
							RST|ACK, NULL, 0);
			break;
			default :
			break;
		}	
	}
	esix_sockets[socknum].state = CLOSING;
	esix_socket_free_queue(socknum);
	esix_sockets[socknum].state = CLOSED;

	return 0;
}

void esix_socket_free_queue(int socknum)
{
	struct sock_queue *sqe;

	//purge the socket element list, one by one
	while(esix_sockets[socknum].queue != NULL)
	{
		//grab the first available element
		sqe = esix_sockets[socknum].queue;
		//remove the current element
		esix_sockets[socknum].queue = sqe->next_e;
		//free its payload, if any
		if(sqe->qe_type == RECV_PKT || sqe->qe_type == SENT_PKT)
			free(sqe->data);
		//finally free it.
		free(sqe);
	}
}

int send(const int socknum, const void *buf, const int len, const uint8_t flags)
{
	//send can be used with both TCP or UDP sockets but in case of
	//UDP we need to make sure we're in connected state
	if(esix_sockets[socknum].state != ESTABLISHED)
		return -1;

	if(esix_sockets[socknum].proto == SOCK_STREAM)
	{
		//queue the segment first, 
		//if it fails, bail out and tell the user.
		if(esix_queue_data(socknum, buf, len, NULL, OUT) < 0)
			return 0;

		//now that we made sure we saved it, try to send it.
		//we can always retransmit it if needed.
		esix_tcp_send(&esix_sockets[socknum].laddr, 
					&esix_sockets[socknum].raddr,
					esix_sockets[socknum].lport,
					esix_sockets[socknum].rport,
					esix_sockets[socknum].seqn,
					esix_sockets[socknum].ackn,
					PSH|ACK, buf, len);

		esix_sockets[socknum].seqn+= len;
		esix_sockets[socknum].rexmit_date = esix_get_time() + 2;

		return len;
	}
	else if(esix_sockets[socknum].proto == SOCK_DGRAM)
	{
		//not saving sent UDP packets
		esix_udp_send(&esix_sockets[socknum].laddr,
					&esix_sockets[socknum].raddr,
					esix_sockets[socknum].lport,
					esix_sockets[socknum].rport,
					buf, len);
		return len;
	}
	else
		//should never happen
		return -1;
}

int esix_port_available(const uint16_t port)
{
	int i=0;
	while(i<ESIX_MAX_SOCK)
	{
		if(esix_sockets[i].state != CLOSED &&
			esix_sockets[i].lport == port)
			return -1;
		i++;
	}
	return 0;
}

//expires every sent packet with (seq number + payload_len) < ackn
int esix_socket_expire_e(int s, uint32_t ackn)
{
	int i=0;
	struct sock_queue *sqe, *prev_sqe, *tmp;
	sqe = prev_sqe = esix_sockets[s].queue;

	//find the first available eligible packet in queue
	//and keep a reference to the previous queue element
	while(sqe != NULL)
	{
		if(sqe->qe_type == SENT_PKT)
		{
			if(sqe->seqn + sqe->data_len <= ackn)
			{
				tmp = sqe;
				//first element needs special treatment
				if(esix_sockets[s].queue == sqe)
					esix_sockets[s].queue = sqe->next_e;
				else
					prev_sqe->next_e = sqe->next_e;
		
				//update pointer to move forward
					sqe 	= sqe->next_e;

				//free the element and its data
				free(tmp->data);
				free(tmp);
	
				i++;
			}
			else 
				break; //we're done here.
		}
		else
		{
			prev_sqe= sqe; 
			sqe 	= sqe->next_e;
		}
	}

	return i;
}
	
//in charge of retransmission / time outs
void esix_socket_housekeep()
{
	int s;
	struct sock_queue *sqe;
	for(s=0; s<ESIX_MAX_SOCK; s++)
	{
		//either retransmission is disabled or scheduled for
		//a later time on this socket
		if(esix_sockets[s].rexmit_date == 0 ||
			esix_sockets[s].rexmit_date > esix_get_time())
			continue;

		//show time. find the first available packet and resend it.
		if((sqe = esix_socket_find_e(s, SENT_PKT, KEEP)) != NULL) 
		{
			if(esix_get_time() - sqe->t_sent > MAX_RETX_TIME)
			{
				//we've been trying far too long
				esix_tcp_send(&esix_sockets[s].laddr, 
						&esix_sockets[s].raddr, esix_sockets[s].lport,
						esix_sockets[s].rport, esix_sockets[s].seqn,
						esix_sockets[s].ackn, RST|ACK, NULL, 0);
				esix_sockets[s].state = CLOSING;
				esix_socket_free_queue(s);
				esix_sockets[s].state = CLOSED;
				continue;
			} 

			//first update the retransmission date
			//exp backoff fashion
			esix_sockets[s].rexmit_date = esix_get_time() + 
				((esix_get_time() - sqe->t_sent)^2);

			esix_tcp_send(&esix_sockets[s].laddr, 
				&esix_sockets[s].raddr, esix_sockets[s].lport,
				esix_sockets[s].rport,	sqe->seqn,
				esix_sockets[s].ackn,	PSH|ACK, 
				sqe->data, sqe->data_len);

		}
		else
		{
			//no more packet to rexmit : either the ACKs we received
			//triggered enough retransmissions or evicted multiple packets from the queue.
			//disable retransmission.
			esix_sockets[s].rexmit_date = 0;
		}
	}
}

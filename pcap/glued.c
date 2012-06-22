/**
 * @file
 * Libpcap glue.
 *
 * @section LICENSE
 * Copyright (c) 2012, Floris Chabert. All rights reserved.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pcap.h>
#include <esix.h>

#define MAC { 0x0080, 0xc580, 0xc53a }

static pcap_t *pcap_handle;
static u16_t mac[3] = MAC;

void *esix_w_malloc(int size)
{
	return malloc(size);
}

void esix_w_free(void *ptr)
{
	free(ptr);
}

u32_t esix_w_get_time(void)
{
	return 0;
}

void esix_w_send_packet(u16_t lla[3], void *packet, int len)
{
	int sent;
	u16_t *ether_frame;
	int i;
	
	ether_frame = malloc(len + 6+6+2);
	if (!ether_frame) {
		fprintf(stderr, "Error: Can't allocate memory\n");
	}

	for (i = 0; i < 3; i++) {
		*ether_frame++ = lla[i]; // destination MAC
	}
	for (i = 0; i < 3; i++) {
		*ether_frame++ = hton16(mac[i]); // source MAC
	}
	*ether_frame++ = 0xdd86; // ethertype
	memcpy(ether_frame, packet, len); // payload
	len += 6+6+2; // add ethernet header
	ether_frame -= 7;

	sent = pcap_inject(pcap_handle, ether_frame, len);
	if (sent == len) {
		printf("Injected packet!\n");
	}
	else {
		if (sent >= 0) {
			fprintf(stderr, "Error: Only %d bytes of %d injected\n", sent, len);
		}
		else {
			fprintf(stderr, "Error: Can't inject a packet\n");
		}
		pcap_breakloop(pcap_handle);
	}
}

static void process_packet(u_char *user, const struct pcap_pkthdr *header, const u_char *bytes)
{
	int for_us = 1;
	int i;

	if (*(u16_t *)bytes == 0x3333) {
		bytes += 6; // multicast, skip destination MAC
	}
	else {
		for (i = 0; i < 3; i++) {
			if (*((u16_t *)bytes++) != hton16(mac[i]))
				for_us = 0;
		}
	}
	if (for_us) {
		bytes += 6; // skip source MAC

		if (*((u16_t *)bytes++) == 0xdd86) {
			printf("Got IPv6 packet!\n");
			esix_ip_process(bytes, header->len);
		}
		else {
			printf("Ignoring non-IPv6 packet %04x.\n", *((u16_t *)bytes+1));
		}
	}
	else {
		printf("Ignoring packet, not for us.\n");
	}
}

static void handle_signal(int sig)
{
	static int exiting = 0;
	if (!exiting) {
		exiting = 1;
		fprintf(stderr, "Exiting...\n");
		pcap_breakloop(pcap_handle);
	}
}

static void init_stack(void)
{
	esix_init(mac);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	char *dev;
	char errbuf[PCAP_ERRBUF_SIZE];
	int err;

	if (geteuid() != 0) {
		fprintf(stderr, "You need to be root to open the default device.\n");
		goto out;
	}

	dev = pcap_lookupdev(errbuf);
	if (!dev) {
		fprintf(stderr, "Error: Couldn't find the default device: %s\n", errbuf);
		goto out;
	}

	pcap_handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if (!pcap_handle) {
		fprintf(stderr, "Error: Couldn't open device %s: %s\n", dev, errbuf);
		goto out;
	}

	signal(SIGINT, handle_signal);

	init_stack();

	printf("Hooked to %s\n", dev);

	err = pcap_loop(pcap_handle, -1, process_packet, NULL);
	if (err == -1) {
		pcap_perror(pcap_handle, "Error: ");
		goto close;
	}

	ret = EXIT_SUCCESS;

close:
	pcap_close(pcap_handle);
out:
	return ret;
}
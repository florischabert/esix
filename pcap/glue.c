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
#include <sys/time.h>
#include <pthread.h>

#define MAC_ADDR { 0x00, 0x80, 0xc5, 0x80, 0xc5, 0x3a }

static pcap_t *pcap_handle;

void esix_send(void *packet, int len)
{
	int sent;

	sent = pcap_inject(pcap_handle, packet, len);
	if (sent != len) {
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
	esix_enqueue((void *)bytes, header->len);
}

static void handle_signal(int sig)
{
	static int exiting = 0;
	if (!exiting) {
		exiting = 1;
		fprintf(stderr, "\nExiting...\n");
		pcap_breakloop(pcap_handle);
	}
}

uint64_t esix_gettime(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	return tp.tv_sec * 1000000000 + tp.tv_usec * 1000;
}

struct esix_sem_t {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int should_run;
};

int esix_sem_init(esix_sem_t *sem)
{
	sem = malloc(sizeof *sem);

	pthread_cond_init(&sem->cond, NULL);
	pthread_mutex_init(&sem->mutex, NULL);
	sem->should_run = 0;

	return 0;
}

void esix_sem_wait(esix_sem_t *sem, uint64_t abstime)
{
	struct timespec ts;
	ts.tv_sec = abstime / 1000000000;
	ts.tv_nsec = abstime % 1000000000;

	pthread_mutex_lock(&sem->mutex);
	if (!sem->should_run) {
		pthread_cond_timedwait(&sem->cond, &sem->mutex, &ts);
	}
	sem->should_run = 0;
	pthread_mutex_unlock(&sem->mutex);
}

void esix_sem_signal(esix_sem_t *sem)
{
	pthread_mutex_lock(&sem->mutex);
	sem->should_run = 1;
	pthread_mutex_unlock(&sem->mutex);

	pthread_cond_signal(&sem->cond);
}

void esix_sem_destroy(esix_sem_t *sem)
{
	pthread_cond_broadcast(&sem->cond);
	pthread_cond_destroy(&sem->cond);
	pthread_mutex_destroy(&sem->mutex);

	free(sem);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	char *dev;
	char errbuf[PCAP_ERRBUF_SIZE];
	int err;
	esix_lla lla = MAC_ADDR;

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

	printf("Hooked on %s\n", dev);

	err = esix_init(lla);
	if (err) {
		fprintf(stderr, "Error: Couldn't initialize esix\n");
		goto pcap_close;
	}

	err = pcap_loop(pcap_handle, -1, process_packet, NULL);
	if (err == -1) {
		pcap_perror(pcap_handle, "Error: ");
		goto esix_destroy;
	}

	esix_workloop();
	
	ret = EXIT_SUCCESS;

esix_destroy:
	esix_destroy();
pcap_close:
	pcap_close(pcap_handle);
out:
	return ret;
}

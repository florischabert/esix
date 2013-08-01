/**
 * @file
 * Unix glue.
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
#include <sys/time.h>
#include <pthread.h>

#include "glue.h"

void *esix_malloc(size_t len)
{
	return malloc(len);
}

void esix_free(void *ptr)
{
	return free(ptr);
}

uint64_t esix_gettime(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	return tp.tv_sec * 1000000000 + tp.tv_usec * 1000;
}

struct esix_mutex_t {
	pthread_mutex_t mutex;
};

esix_mutex_t *esix_mutex_create()
{
	esix_mutex_t *mutex;

	mutex = esix_malloc(sizeof *mutex);
	pthread_mutex_init(&mutex->mutex, NULL);

	return mutex;
}

void esix_mutex_lock(esix_mutex_t *mutex)
{
	pthread_mutex_lock(&mutex->mutex);
}

void esix_mutex_unlock(esix_mutex_t *mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
}

void esix_mutex_destroy(esix_mutex_t *mutex)
{
	pthread_mutex_destroy(&mutex->mutex);
	esix_free(mutex);
}

struct esix_sem_t {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int should_run;
};

esix_sem_t *esix_sem_create()
{
	esix_sem_t *sem;

	sem = esix_malloc(sizeof *sem);

	pthread_cond_init(&sem->cond, NULL);
	pthread_mutex_init(&sem->mutex, NULL);
	sem->should_run = 0;

	return sem;
}

void esix_sem_wait(esix_sem_t *sem, uint64_t delay_ns)
{
	struct timespec ts;
	uint64_t abstime_ns;
	
	abstime_ns = esix_gettime() + delay_ns;

	ts.tv_sec = abstime_ns / 1000000000;
	ts.tv_nsec = abstime_ns % 1000000000;

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

	esix_free(sem);
}

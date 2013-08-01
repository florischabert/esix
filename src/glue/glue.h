/**
 * @file
 * esix ipv6 stack main file.
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

#ifndef _ESIX_GLUE_H
#define _ESIX_GLUE_H

typedef struct esix_mutex_t esix_mutex_t;
typedef struct esix_sem_t esix_sem_t;

esix_mutex_t *esix_mutex_create(void);
void esix_mutex_destroy(esix_mutex_t *mutex);
void esix_mutex_lock(esix_mutex_t *mutex);
void esix_mutex_unlock(esix_mutex_t *mutex);

esix_sem_t *esix_sem_create(void);
void esix_sem_destroy(esix_sem_t *sem);
void esix_sem_wait(esix_sem_t *sem, uint64_t delay_ns);
void esix_sem_signal(esix_sem_t *sem);

uint64_t esix_gettime(void);

void *esix_malloc(size_t len);
void esix_free(void *ptr);


#endif

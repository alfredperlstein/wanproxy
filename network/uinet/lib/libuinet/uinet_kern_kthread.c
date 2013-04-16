/*-
 * Copyright (c) 2010 Kip Macy
 * All rights reserved.
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Derived in part from libplebnet's pn_compat.c.
 *
 */

#undef _KERNEL
#include <sys/param.h>
#include <sys/types.h>
#include <sys/kthread.h>
#include <sys/mman.h>
#include <sys/refcount.h>
#include <sys/ucred.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/stdint.h>
#include <sys/uio.h>

#define _KERNEL
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sx.h>
#include <sys/linker.h>
#undef _KERNEL

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

__thread struct thread *pcurthread;

struct pthread_start_args 
{
	struct thread *psa_td;
	void (*psa_start_routine)(void *);
	void *psa_arg;
};


int
_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	       void *(*start_routine)(void *), void *arg);

void
uinet_init_thread0(void);


static void *
pthread_start_routine(void *arg)
{
	struct pthread_start_args *psa = arg;
	printf("Creating new thread, pcurthread = %p\n", psa->psa_td);
	pcurthread = psa->psa_td;
	pcurthread->td_proc = &proc0;
	psa->psa_start_routine(psa->psa_arg);
	free(psa->psa_td);
	free(psa);

	return (NULL);
}

/*
 * N.B. The flags are ignored.  Namely RFSTOPPED is not honored and threads
 * are started right away.
 */
int
kthread_add(void (*start_routine)(void *), void *arg, struct proc *p,  
    struct thread **tdp, int flags, int pages,
    const char *str, ...)
{
	int error;
	pthread_t thread;
	pthread_attr_t attr;
	struct pthread_start_args *psa;
	struct thread *td;
	struct mtx *lock;
	pthread_cond_t *cond; 

	td = malloc(sizeof(struct thread));
	if (tdp)
		*tdp = td;

	psa = malloc(sizeof(struct pthread_start_args));
	lock = malloc(sizeof(struct mtx));
	cond = malloc(sizeof(pthread_cond_t));
	pthread_cond_init(cond, NULL);
	mtx_init(lock, "thread_lock", NULL, MTX_DEF);
	td->td_lock = lock;
	td->td_sleepqueue = (void *)cond;

	psa->psa_start_routine = start_routine;
	psa->psa_arg = arg;
	psa->psa_td = td;
	
	pthread_attr_init(&attr); 
	error = _pthread_create(&thread, &attr, pthread_start_routine, psa);
	td->td_wchan = thread;
	return (error);
}

void
kthread_exit(void)
{
	pthread_exit(NULL);
}

/*
 * N.B. This doesn't actually create the proc if it doesn't exist. It 
 * just uses proc0. 
 */
int
kproc_kthread_add(void (*start_routine)(void *), void *arg,
    struct proc **p,  struct thread **tdp,
    int flags, int pages,
    const char * procname, const char *str, ...)
{
	int error;
	pthread_t thread;
	struct thread *td;
	pthread_attr_t attr;
	struct pthread_start_args *psa;
	struct mtx *lock;
	pthread_cond_t *cond; 

	*tdp = td = malloc(sizeof(struct thread));
	psa = malloc(sizeof(struct pthread_start_args));
	lock = malloc(sizeof(struct mtx));
	cond = malloc(sizeof(pthread_cond_t));
	pthread_cond_init(cond, NULL);
	mtx_init(lock, "thread_lock", NULL, MTX_DEF);
	td->td_lock = lock;
	td->td_sleepqueue = (void *)cond;
	psa->psa_start_routine = start_routine;
	psa->psa_arg = arg;
	psa->psa_td = td;
	
	pthread_attr_init(&attr); 
	error = _pthread_create(&thread, &attr, pthread_start_routine, psa);
	td->td_wchan = thread;
	return (error);
}


void
uinet_init_thread0(void)
{

	pcurthread = &thread0;
}

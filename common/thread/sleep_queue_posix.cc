/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <pthread.h>

#include <common/thread/mutex.h>
#include <common/thread/sleep_queue.h>
#include <common/thread/thread.h>

#include <common/time/time.h>

#include "mutex_posix.h"
#include "sleep_queue_posix.h"

SleepQueue::SleepQueue(const std::string& name, Mutex *mutex)
: name_(name),
  mutex_(mutex),
  state_(new SleepQueueState(mutex->state_))
{ }

SleepQueue::~SleepQueue()
{
#if 0 /* XXX What about extern SleepQueues?  */
	ASSERT_LOCK_OWNED(log_, mutex_);
#endif

	if (state_ != NULL) {
		delete state_;
		state_ = NULL;
	}
}

void
SleepQueue::signal(void)
{
	ASSERT_LOCK_OWNED("/sleep/queue", mutex_);
	state_->signal();
	ASSERT_LOCK_OWNED("/sleep/queue", mutex_);
}

void
SleepQueue::wait(const NanoTime *deadline)
{
	ASSERT_LOCK_OWNED("/sleep/queue", mutex_);
	state_->wait(deadline);
	ASSERT_LOCK_OWNED("/sleep/queue", mutex_);
}

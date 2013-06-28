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

#ifndef	EVENT_EVENT_SYSTEM_H
#define	EVENT_EVENT_SYSTEM_H

#include <event/event_poll.h>
#include <event/event_thread.h>
#include <event/timeout_thread.h>

/*
 * XXX
 * This is kind of an awful shim while we move
 * towards something thread-oriented.
 */

class EventSystem {
	EventThread td_;
	EventPoll poll_;
	TimeoutThread timeout_;
	std::deque<Thread *> threads_;
private:
	EventSystem(void)
	: td_(),
	  poll_(),
	  timeout_()
	{ }

	~EventSystem()
	{ }

public:
	Action *poll(const EventPoll::Type& type, int fd, EventCallback *cb)
	{
		return (poll_.poll(type, fd, cb));
	}

	Action *register_interest(const EventInterest& interest, SimpleCallback *cb)
	{
		return (td_.register_interest(interest, cb));
	}

	Action *schedule(CallbackBase *cb)
	{
		return (td_.schedule(cb));
	}

	Action *timeout(unsigned ms, SimpleCallback *cb)
	{
		return (timeout_.timeout(ms, cb));
	}

	void thread_wait(Thread *td)
	{
		threads_.push_back(td);
	}

	void start(void)
	{
		td_.start();
		thread_wait(&td_);

		poll_.start();
		thread_wait(&poll_);

		timeout_.start();
		thread_wait(&timeout_);
	}

	void join(void)
	{
		while (!threads_.empty()) {
			threads_.front()->join();
			threads_.pop_front();
		}
	}

	void stop(void)
	{
		std::deque<Thread *>::const_iterator it;
		for (it = threads_.begin(); it != threads_.end(); ++it) {
			Thread *td = *it;
			td->stop();
		}
	}

	static EventSystem *instance(void)
	{
		static EventSystem instance;

		return (&instance);
	}
};

#endif /* !EVENT_EVENT_SYSTEM_H */

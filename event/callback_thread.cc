/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#include <event/callback_thread.h>

#include <event/event_callback.h>
#include <event/event_system.h>

CallbackThread::CallbackThread(const std::string& name)
: Thread(name),
  log_("/callback/thread/" + name),
  mtx_(name),
  sleepq_(name, &mtx_),
  idle_(false),
  queue_(),
  inflight_(NULL)
{ }

/*
 * NB:
 * Unless the caller itself is running in the CallbackThread, it needs to
 * acquire a lock in its cancel path as well as around this schedule
 * call to avoid races when dispatching deferred callbacks.
 */
Action *
CallbackThread::schedule(CallbackBase *cb)
{
	mtx_.lock();
	bool need_wakeup = queue_.empty();
	queue_.push_back(cb);
	if (need_wakeup && idle_)
		sleepq_.signal();
	mtx_.unlock();

	return (cancellation(this, &CallbackThread::cancel, cb));
}

/*
 * XXX
 * Unless a callback can only be cancelled from within the CallbackThread,
 * there is a race on inflight callbacks.
 *
 * 	Thread X			CallbackThread
 * 	lock(Foo)
 * 	cb = callback(Foo, Foo::handle)
 * 	a = schedule(cb)
 * 	unlock(Foo)
 * 					lock(mtx_)
 * 					cb = queue_.front()
 * 					inflight_ = cb
 * 					unlock(mtx_)
 * 	lock(Foo)			cb->execute()
 * 					Foo::handle()
 * 	a->cancel()			lock(Foo) -- blocks
 * 	lock(mtx_)
 * 	inflight_ = NULL
 * 	unlock(mtx_)
 * 	a = NULL
 * 	unlock(Foo)			lock(Foo) -- completes
 * 					a->cancel() -- NULL deref
 *
 * Note that other bad things are possible than the NULL deref.  Foo may no longer exist.
 *
 * If we keep mtx_ locked, then the race is limited to the NULL deref, but performance
 * suffers.  If callbacks had mutexes associated with them and we could interlock, this
 * would be better.
 *
 * Changing how Action works could help, too.  More details on that later.
 */
void
CallbackThread::cancel(CallbackBase *cb)
{
	mtx_.lock();
	if (inflight_ == cb) {
		inflight_ = NULL;
		mtx_.unlock();
		return;
	}

	std::deque<CallbackBase *>::iterator it;
	for (it = queue_.begin(); it != queue_.end(); ++it) {
		if (*it != cb)
			continue;
		queue_.erase(it);
		mtx_.unlock();
		delete cb;
		return;
	}

	NOTREACHED(log_);
}

void
CallbackThread::main(void)
{
	mtx_.lock();
	for (;;) {
		if (queue_.empty()) {
			idle_ = true;
			for (;;) {
				if (stop_) {
					mtx_.unlock();
					return;
				}
				sleepq_.wait();
				if (queue_.empty())
					continue;
				idle_ = false;
				break;
			}
		}

		while (!queue_.empty()) {
			CallbackBase *cb = queue_.front();
			queue_.pop_front();
			inflight_ = cb;
			mtx_.unlock();

			/*
			 * XXX
			 * Could batch these to improve throughput with lots of
			 * callbacks at once.  Have a set of in-flight callbacks
			 * as well as the current one, and a second lock for the
			 * callbacks in-flight, and check that as a last resort
			 * in the cancel path.
			 *
			 * Right now fixated on performance with one callback at
			 * a time, for which it won't help a lot, so it's worth
			 * not overthinking.  Moving to lockless append on a
			 * per-thread basis would be reasonable, and then a
			 * lockless move of the whole queue out at a time.  That
			 * would avoid the serious lock overhead involved here.
			 */
			cb->execute();
			delete cb;

			mtx_.lock();
			if (inflight_ != NULL)
				HALT(log_) << "Callback not cancelled in execution.";
		}
	}
}

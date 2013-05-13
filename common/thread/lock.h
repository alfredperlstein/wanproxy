/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_THREAD_LOCK_H
#define	COMMON_THREAD_LOCK_H

class Lock {
	std::string name_;
protected:
	Lock(const std::string& name)
	: name_(name)
	{ }
public:
	virtual ~Lock()
	{ }

	virtual void assert_owned(bool, const LogHandle&, const std::string&, unsigned, const std::string&) = 0;
	virtual void lock(void) = 0;
	virtual void unlock(void) = 0;
};

#define	ASSERT_LOCK_OWNED(log, lock)					\
	((lock)->assert_owned(true, log, __FILE__, __LINE__, __PRETTY_FUNCTION__))
#define	ASSERT_LOCK_NOT_OWNED(log, lock)				\
	((lock)->assert_owned(false, log, __FILE__, __LINE__, __PRETTY_FUNCTION__))

class ScopedLock {
	Lock *lock_;
public:
	ScopedLock(Lock *lock)
	: lock_(lock)
	{
		ASSERT("/scoped/lock", lock_ != NULL);
		lock_->lock();
	}

	~ScopedLock()
	{
		if (lock_ != NULL) {
			lock_->unlock();
			lock_ = NULL;
		}
	}

	void acquire(Lock *lock)
	{
		ASSERT("/scoped/lock", lock_ == NULL);
		lock_ = lock;
		lock_->lock();
	}

	void drop(void)
	{
		ASSERT("/scoped/lock", lock_ != NULL);
		lock_->unlock();
		lock_ = NULL;
	}
};

#endif /* !COMMON_THREAD_LOCK_H */

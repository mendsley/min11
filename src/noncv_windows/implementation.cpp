/*
 * Copyright 2014 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "min11/mutex.h"
#include "min11/condition_variable.h"
#include "min11/atomic_counter.h"

#if defined(_XBOX_VER)
#	include <xtl.h>
#else
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#endif
#include <limits.h>

using namespace min11;
using namespace min11::detail;

struct min11::detail::mutex_internal
{
	CRITICAL_SECTION cs;
};

mutex::mutex()
	: m(new mutex_internal)
{
	InitializeCriticalSection(&m->cs);
}

mutex::~mutex()
{
	DeleteCriticalSection(&m->cs);
	delete m;
}

void mutex::lock()
{
	EnterCriticalSection(&m->cs);
}

bool mutex::try_lock()
{
	return TRUE == TryEnterCriticalSection(&m->cs);
}

void mutex::unlock()
{
	LeaveCriticalSection(&m->cs);
}

///////////////////////////////////////////////////////////////////////////////

struct min11::detail::condition_variable_internal
{
	volatile long waiters;
	HANDLE sema;
};

condition_variable::condition_variable()
	: cv(new condition_variable_internal)
{
	cv->sema = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	cv->waiters = 0;
}

condition_variable::~condition_variable()
{
	CloseHandle(cv->sema);
	delete cv;
}

void condition_variable::wait(unique_lock<mutex>& lock)
{
	InterlockedIncrementRelease(&cv->waiters);
	lock.mutex()->unlock();
	WaitForSingleObject(&cv->sema, INFINITE);
	lock.mutex()->lock();
}

static void signal_cv(condition_variable_internal* cv, bool wakeAll)
{
	for (;;)
	{
		long old = cv->waiters;
		if (old == 0)
		{
			return;
		}
		
		long waiters = wakeAll ? 0 : (old - 1);
		if (old == InterlockedCompareExchangeRelease(&cv->waiters, waiters, old))
		{
			ReleaseSemaphore(cv->sema, old-waiters, NULL);
			return;
		}
	}
}

void condition_variable::notify_one()
{
	signal_cv(cv, false);
}

void condition_variable::notify_all()
{
	signal_cv(cv, true);
}

///////////////////////////////////////////////////////////////////////////////

atomic_counter::atomic_counter(long initial)
	: value(initial)
{
}

void atomic_counter::incr()
{
	InterlockedIncrement(&value);
}

long atomic_counter::decr()
{
	return InterlockedDecrement(&value);
}

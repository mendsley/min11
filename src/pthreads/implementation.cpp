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

#include <pthread.h>

using namespace min11;
using namespace min11::detail;

struct min11::detail::mutex_internal
{
	pthread_mutex_t mtx;
};

mutex::mutex()
	: m(new mutex_internal)
{
	pthread_mutex_init(&m->mtx, NULL);
}

mutex::~mutex()
{
	pthread_mutex_destroy(&m->mtx);
	delete m;
}

void mutex::lock()
{
	pthread_mutex_lock(&m->mtx);
}

bool mutex::try_lock()
{
	pthread_mutex_trylock(&m->mtx);
}

void mutex::unlock()
{
	pthread_mutex_unlock(&m->mtx);
}

///////////////////////////////////////////////////////////////////////////////

struct min11::detail::condition_variable_internal
{
	pthread_cond_t cv;
};

condition_variable::condition_variable()
	: cv(new condition_variable_internal)
{
	pthread_cond_init(&cv->cv, NULL);
}

condition_variable::~condition_variable()
{
	pthread_cond_destroy(&cv->cv);
	delete cv;
}

void condition_variable::wait(unique_lock<mutex>& lock)
{
	pthread_cond_wait(&cv->cv, &lock.mutex()->m->mtx);
}

void condition_variable::notify_one()
{
	pthread_cond_signal(&cv->cv);
}

void condition_variable::notify_all()
{
	pthread_cond_broadcast(&cv->cv);
}

///////////////////////////////////////////////////////////////////////////////

atomic_counter::atomic_counter(long initial)
	: value(initial)
{
}

void atomic_counter::incr()
{
	__sync_fetch_and_add(&value, 1);
}

long atomic_counter::decr()
{
	return __sync_fetch_and_sub(&value, 1) - 1;
}

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

#ifndef MIN11__MUTEX_H
#define MIN11__MUTEX_H

namespace min11
{
	class condition_variable;
	namespace detail { struct mutex_internal; }
	
	/**
	 * Minimal emulation for std::mutex
	 */
	class mutex
	{
		friend class condition_variable;
	public:
		mutex();
		~mutex();
		
		void lock();
		bool try_lock();
		void unlock();
		
	private:
		mutex(const mutex&); // = delete;
		mutex& operator=(const mutex&); // = delete
		
		detail::mutex_internal* m;
	};
	
	/**
	 * Minimal emulation for std::unique_lock
	 */
	template<typename M>
	class unique_lock
	{
	public:
		explicit unique_lock(M& mutex)
			: mtx(&mutex)
		{
			mtx->lock();
		}

		~unique_lock()
		{
			mtx->unlock();
		}

		M* mutex() const
		{
			return mtx;
		}

	private:
		unique_lock(const unique_lock&); // = delete;
		unique_lock& operator=(const unique_lock&); // = delete;

		M* mtx;
	};
}

#endif // MIN11__MUTEX_H

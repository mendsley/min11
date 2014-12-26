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
 
#ifndef MIN11__FUTURE_H
#define MIN11__FUTURE_H

#include "config.h"
#include "atomic_counter.h"
#include "condition_variable.h"
#include "mutex.h"

#if MIN11_HAS_EXCEPTIONS
#	include <stdexcept>
#endif

namespace min11
{	
	template<typename T> class future;
	template<typename T> class shared_future;
	template<typename T> class promise;
	namespace detail { template<typename T> class future_state; }
	namespace detail { template<typename T> class movable_future; }

	/**
	 * Create a movable reference to a future
	 */
	template<typename T>
	detail::movable_future<T> move(future<T>& f)
	{
		return detail::movable_future<T>::from_future(f);
	}

	namespace detail
	{
		/**
		 * helper class to represent a "movable" future. The expression
		 * detail::movable_future<T>(ftr) should be roughly equivalent to an
		 * rvalue reference to `ftr': similar to the expression std::move(ftr)
		 */
		template<typename T>
		class movable_future
		{
			friend future<T>;
			friend shared_future<T>;
			friend promise<T>;

		public:
			movable_future(const movable_future& rhs)
				: state(rhs.state)
			{
				if (state)
					state->add_ref();
			}

			~movable_future()
			{
				if (state)
					state->dec_ref();
			}

			static movable_future<T> from_future(future<T>& f)
			{
				return movable_future<T>(f);
			}

		private:
			movable_future& operator=(const movable_future&); // = delete;

			explicit movable_future(future<T>& f)
				: state(f.state)
			{
				f.state = 0;
			}
			
			explicit movable_future(detail::future_state<T>* state)
				: state(state)
			{
				state->add_ref();
			}
			
			mutable detail::future_state<T>* state;
		};
	}

	/**
	 * Minimal emulation for std::future_error
	 */
#if MIN11_HAS_EXCEPTIONS
	struct future_error
		: public std::logic_error
	{
		future_error(const char* message)
			: std::logic_error(message)
		{
		}
	};
#endif

	namespace detail
	{
		static inline void throw_future_error(const char* message)
		{
#if MIN11_HAS_EXCEPTIONS
			throw future_error(message);
#else
			MIN11_THROW_EXCEPTION(message);
#endif
		}
	}

	/**
	 * Minimal emulation for std::future
	 */
	template<typename T>
	class future
	{
		friend promise<T>;
		friend shared_future<T>;
		friend detail::movable_future<T>;
	public:
		future()
			: state(0)
		{
		}

		future(const detail::movable_future<T>& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}
		
		~future()
		{
			if (state)
				state->dec_ref();
		}
		
		future& operator=(detail::movable_future<T> rhs)
		{
			state = rhs.state;
			rhs.state = 0;
		}
		
		bool valid() const
		{
			return state && !state->retrieved;
		}
		
		T& get()
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");
				
			return state->get_value(false);
		}
		
		void wait() const
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");

			state->wait();
		}
		
		shared_future<T> share()
		{
			return shared_future<T>(move(*this));
		}

	private:
		future(const future&); // = delete;
		future& operator=(const future&); // = delete;
	
		detail::future_state<T>* state;
	};
	
	/**
	 * Minimal emulation for std::shared_future
	 */
	template<typename T>
	class shared_future
	{
	public:
		shared_future()
			: state(0)
		{
		}

		shared_future(detail::movable_future<T>& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}
		
		shared_future(const shared_future<T>& other)
			: state(other.state)
		{
			if (state)
				state->add_ref();
		}
		
		~shared_future()
		{
			if (state)
				state->dec_ref();
		}
		
		shared_future& operator=(const shared_future& other)
		{
			if (rhs.state)
				rhs.state->add_ref();
			if (state)
				state->dec_ref();
			state = rhs.state;
		}
		
		shared_future& operator=(detail::movable_future<T>& rhs)
		{
			if (state)
				state->dec_ref();

			state = rhs.state;
			rhs.state = 0;
		}
		
		bool valid() const
		{
			return state != 0;
		}
		
		void wait() const
		{
			if (!valid())
				detail::throw_future_error("future has no state object");

			state->wait();
		}
		
		const T& get() const
		{
			if (!valid())
				detail::throw_future_error("future has no state object");

			return state->get_value(true);
		}

	private:
		detail::future_state<T>* state;
	};
	
	/**
	 * Minimal emulation of std::promise
	 */
	template<typename T>
	class promise
	{
	public:
		promise()
			: state(new detail::future_state<T>)
		{
		}
		
		~promise()
		{
			state->dec_ref();
		}
		
		void set_value(const T& value)
		{
			state->set_value(value);
		}
		
		detail::movable_future<T> get_future()
		{
			return detail::movable_future<T>(state);
		}
	private:
		promise(const promise&); // = delete;
		promise& operator=(const promise&); // = delete;

		detail::future_state<T>* state;
	};
	
	namespace detail
	{
		template<typename T>
		class future_state
		{
			friend future<T>;
		public:
			future_state()
				: refcnt(1)
				, retrieved(false)
				, ready(false)
			{
			}
			
			void add_ref()
			{
				refcnt.incr();
			}
			
			void dec_ref()
			{
				if (0 == refcnt.decr())
				{
					delete this;
				}
			}
			
			void set_value(const T& value)
			{
				unique_lock<mutex> lock(mtx);
				set_value_with_lock(value);
			}
			
			T& get_value(bool allow_multiple_gets)
			{
				unique_lock<mutex> lock(mtx);
				if (retrieved && !allow_multiple_gets)
					detail::throw_future_error("future already retreived");
					
				retrieved = true;
				while (!ready)
					cond.wait(lock);
				
				return value;
			}
			
			void wait()
			{
				unique_lock<mutex> lock(mtx);
				while (!ready)
					cond.wait(lock)
			}

		private:
			future_state(const future_state&); // = delete;
			future_state& operator=(const future_state&); // = delete;
			
			void set_value_with_lock(const T& v)
			{
				if (ready)
					detail::throw_future_error("future state already set");
					
				value = v;
				ready = true;
				cond.notify_all();
			}

			detail::atomic_counter refcnt;
			T value;
			mutex mtx;
			condition_variable cond;
			bool retrieved;
			volatile bool ready;
		};
	}
}

#endif // MIN11__FUTURE_H

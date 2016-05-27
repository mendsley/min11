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

#if MIN11_HAS_RVALREFS
#	include <utility> // std::move
#endif

#if MIN11_HAS_FUTURE_CONTINUATIONS
#	include <functional> // std::function
#endif

namespace min11
{	
	template<typename T> class future;
	template<typename T> class shared_future;
	template<typename T> class promise;
	namespace detail { template<typename T> class future_state; }
	namespace detail { template<typename T, typename Data> class future_state_data; }
	namespace detail { template<typename T> class promise_holder; }
	namespace detail { template<typename T, typename Data> class promise_holder_data; }
#if !MIN11_HAS_RVALREFS
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
			friend class future<T>;
			friend class shared_future<T>;
			friend class promise<T>;

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

			shared_future<T> share()
			{
				return shared_future<T>(*this);
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
#else
	using std::move;
#endif

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

		struct make_ready_helper;
	}

	/**
	 * Minimal emulation for std::future
	 */
	template<typename T>
	class future
	{
		friend class promise<T>;
		friend class shared_future<T>;
		friend detail::make_ready_helper;
#if !MIN11_HAS_RVALREFS
		friend class detail::movable_future<T>;
#endif
	public:
		future()
			: state(0)
		{
		}

#if MIN11_HAS_RVALREFS
		future(future<T>&& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}

#else
		future(const detail::movable_future<T>& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}
#endif
		
		~future()
		{
			if (state)
				state->dec_ref();
		}
		
#if MIN11_HAS_RVALREFS
		future& operator=(future<T>&& rhs)
		{
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#else
		future& operator=(const detail::movable_future<T>& rhs)
		{
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#endif
		
		bool valid() const
		{
			return state && !state->retrieved;
		}
		
		T get()
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");
#if MIN11_HAS_RVALREFS
			return std::move(state->get_value(false));
#else
			return state->get_value(false);
#endif
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

#if MIN11_HAS_FUTURE_CONTINUATIONS
		template<typename Func>
#if MIN11_HAS_RVALREFS
		void set_continuation(Func&& f)
#else
		void set_continuation(const Func& f)
#endif
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");

#if MIN11_HAS_RVALREFS
			state->set_continuation(std::move(f));
#else
			state->set_continuation(f);
#endif
		}
#endif

	private:
		explicit future(const future&); // = delete;
		future& operator=(const future&); // = delete;

		future(detail::future_state<T>* s)
			: state(s)
		{
			if (s)
				s->add_ref();
		}
	
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

#if MIN11_HAS_RVALREFS
		shared_future(future<T>&& rhs)
			: state(0)
		{
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
		}

		shared_future(shared_future<T>&& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}
#else
		shared_future(const detail::movable_future<T>& rhs)
			: state(0)
		{
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
		}
#endif
		
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
		
		shared_future& operator=(const shared_future& rhs)
		{
			if (rhs.state)
				rhs.state->add_ref();
			if (state)
				state->dec_ref();
			state = rhs.state;
			return *this;
		}
		
#if MIN11_HAS_RVALREFS
		shared_future& operator=(future<T>&& rhs)
		{
			if (state)
				state->dec_ref();

			state = 0;
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}

		shared_future& operator=(shared_future<T>&& rhs)
		{
			if (state)
				state->dec_ref();

			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#else
		shared_future& operator=(const detail::movable_future<T>& rhs)
		{
			if (state)
				state->dec_ref();

			state = 0;
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#endif
		
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

	// promise that does not keep the internal state alive
	template<typename T>
	class weak_promise
	{
		friend promise<T>;
	public:
		weak_promise()
			: state(NULL)
		{
		}
		
#if MIN11_HAS_RVALREFS
		void set_value(T&& value)
		{
			state->set_value(std::move(value));
		}
#endif
		void set_value(const T& value)
		{
			state->set_value(value);
		}

	private:
		explicit weak_promise(detail::future_state<T>* state)
			: state(state)
		{
		}

		detail::future_state<T>* state;
	};
	
	/**
	 * Minimal emulation of std::promise
	 */
	template<typename T>
	class promise
	{
		friend detail::promise_holder<T>;
	public:
		promise()
			: state(new detail::future_state<T>)
		{
		}

#if MIN11_HAS_RVALREFS
		promise(promise<T>&& other)
			: state(other.state)
		{
			other.state = 0;
		}
#endif
		
		virtual ~promise()
		{
			state->dec_ref();
		}
		
#if MIN11_HAS_RVALREFS
		void set_value(T&& value)
		{
			state->set_value(std::move(value));
		}
#endif
		void set_value(const T& value)
		{
			state->set_value(value);
		}

		
#if MIN11_HAS_RVALREFS
		future<T> get_future()
		{
			return future<T>(state);
		}
#else
		detail::movable_future<T> get_future()
		{
			return detail::movable_future<T>(state);
		}
#endif

		detail::promise_holder<T> hold()
		{
			return detail::promise_holder<T>(state);
		}

	private:
		promise(const promise&); // = delete;
		promise& operator=(const promise&); // = delete;

	protected:
		promise(detail::future_state<T>* st)
			: state(st)
		{
			state->add_ref();
		}

		weak_promise<T> weak_ref()
		{
			return weak_promise<T>(state);
		}

		detail::future_state<T>* state;
	};

	template<typename T, typename Data>
	class promise_with_data : public promise<T>
	{
		friend detail::promise_holder_data<T, Data>;
	public:
		promise_with_data()
			: promise<T>(new detail::future_state_data<T, Data>)
		{
			promise<T>::state->dec_ref();
		}

#if MIN11_HAS_RVALREFS
		promise_with_data(promise_with_data<T, Data>&& other)
			: promise<T>(std::move(other))
		{
		}

		promise_with_data& operator=(promise_with_data<T, Data>&& other)
		{
			if (other.state)
				other.state->add_ref();
			if (this->state)
				this->state->dec_ref();
			this->state = other.state;
			other.state = NULL;
			return *this;
		}
#endif

		Data& data()
		{
			return static_cast<detail::future_state_data<T, Data>*>(promise<T>::state)->data();
		}

		detail::promise_holder_data<T, Data> hold()
		{
			return detail::promise_holder_data<T, Data>(static_cast<detail::future_state_data<T, Data>*>(promise<T>::state));
		}

		using promise<T>::weak_ref;

	private:
		promise_with_data(detail::future_state_data<T, Data>* state)
			: promise<T>(state)
		{
		}
	};
	
	namespace detail
	{
		template<typename T>
		struct closure_root
		{
			virtual ~closure_root() {}
#if MIN11_HAS_RVALREFS
			virtual void invoke(T&& v) = 0;
#else
			virtual void invoke(const T& v) = 0;
#endif
		};

		template<typename T, typename Func>
		struct closure : closure_root<T>
		{

#if MIN11_HAS_RVALREFS
			closure(Func func)
				: f(std::move(func))
#else
			closure(const Func& func)
				: f(func)
#endif
			{
			}

#if MIN11_HAS_RVALREFS
			virtual void invoke(T&& v)
			{
				f(std::move(v));
			}
#else
			virtual void invoke(const T& v)
			{
				f(v);
			}
#endif

			Func f;
		};

		template<typename T>
		struct closure_storage
		{
			closure_storage()
				: ptr(0)
			{
			}

			~closure_storage()
			{
				release();
			}

			template<typename Func>
#if MIN11_HAS_RVALREFS
			void alloc(Func func)
#else
			void alloc(const Func& func)
#endif
			{
				release();
				if (sizeof(closure<T, Func>) < sizeof(space))
				{
					ptr = new(&space) closure<T, Func>(
#if MIN11_HAS_RVALREFS
						std::move(func)
#else
						func
#endif
					);
				}
				else
				{
					ptr = new closure<T, Func>(
#if MIN11_HAS_RVALREFS
						std::move(func)
#else
						func
#endif
					);
				}
			}

			void release()
			{
				if (static_cast<void*>(ptr) != &space.ptr)
				{
					delete ptr;
				}
				else if (ptr)
				{
					ptr->~closure_root();
				}
			}

			closure_root<T>* ptr;
			union
			{
				long double alignment_;
				void* ptr[3];
			} space;
		};

		template<typename T>
		class future_state
		{
			friend class future<T>;
		public:
			future_state()
				: refcnt(1)
				, retrieved(false)
				, ready(false)
			{
			}

			virtual ~future_state()
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

#if MIN11_HAS_RVALREFS
			void set_value(T&& value)
			{
				unique_lock<mutex> lock(mtx);
				set_value_with_lock(std::move(value));
			}
#endif
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
					
				while (!ready)
					cond.wait(lock);

				if (retrieved && !allow_multiple_gets)
					detail::throw_future_error("future already retreived");

				retrieved = true;
				return value;
			}
			
			void wait()
			{
				unique_lock<mutex> lock(mtx);
				while (!ready)
					cond.wait(lock);
			}

			template<typename Func>
#if MIN11_HAS_RVALREFS
			void set_continuation(Func&& f)
#else
			void set_continuation(const Func& f)
#endif
			{
				unique_lock<mutex> lock(mtx);

				if (cont.ptr)
					detail::throw_future_error("continuation state already set");
				if (retrieved)
					detail::throw_future_error("future already retrieved");

				if (ready)
				{
					retrieved = true;
#if MIN11_HAS_RVALREFS
					f(std::move(value));
#else
					f(value);
#endif
				}
				else
				{
#if MIN11_HAS_RVALREFS
					cont.alloc(std::move(f));
#else
					cont.alloc(f);
#endif
				}
			}

		private:
			future_state(const future_state&); // = delete;
			future_state& operator=(const future_state&); // = delete;
			
#if MIN11_HAS_RVALREFS
			void set_value_with_lock(T&& v)
			{
				if (ready)
					detail::throw_future_error("future state already set");

				value = std::move(v);
				ready = true;
				notify_with_lock();
			}
#endif

			void set_value_with_lock(const T& v)
			{
				if (ready)
					detail::throw_future_error("future state already set");

				value = v;
				ready = true;
				notify_with_lock();
			}

			void notify_with_lock()
			{
#if MIN11_HAS_FUTURE_CONTINUATIONS
				if (cont.ptr)
#if MIN11_HAS_RVALREFS
					cont.ptr->invoke(std::move(value));
#else
					cont.ptr->invoke(value);
#endif
#endif
				cond.notify_all();
			}

			detail::atomic_counter refcnt;
			T value;
			mutex mtx;
			condition_variable cond;
			detail::closure_storage<T> cont;
			bool retrieved;
			volatile bool ready;
		};

		template<typename T, typename Data>
		class future_state_data : public future_state<T>
		{
		public:

			Data& data()
			{
				return data_;
			}

		private:
			Data data_;
		};

		template<>
		class future_state<void>
		{
			friend class future<void>;
			friend class shared_future<void>;
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

			void set_value()
			{
				unique_lock<mutex> lock(mtx);
				set_value_with_lock();
			}

			void get_value(bool allow_multiple_gets)
			{
				unique_lock<mutex> lock(mtx);
				if (retrieved && !allow_multiple_gets)
					detail::throw_future_error("future already retreived");

				while (!ready)
					cond.wait(lock);

				if (retrieved && !allow_multiple_gets)
					detail::throw_future_error("future already retreived");

				retrieved = true;
			}

			void wait()
			{
				unique_lock<mutex> lock(mtx);
				while (!ready)
					cond.wait(lock);
			}

			template<typename Func>
#if MIN11_HAS_RVALREFS
			void set_continuation(Func&& f)
#else
			void set_continuation(const Func& f)
#endif
			{
				unique_lock<mutex> lock(mtx);

				if (cont)
					detail::throw_future_error("continuation state already set");
				if (retrieved)
					detail::throw_future_error("future already retrieved");

				if (ready)
				{
					retrieved = true;
					f();
				}
				else
				{
#if MIN11_HAS_RVALREFS
					cont = std::move(f);
#else
					cont = f;
#endif
				}
			}

		private:
			future_state(const future_state&); // = delete;
			future_state& operator=(const future_state&); // = delete;

			void set_value_with_lock()
			{
				if (ready)
					detail::throw_future_error("future state already set");

				ready = true;
				notify_with_lock();
			}

			void notify_with_lock()
			{
#if MIN11_HAS_FUTURE_CONTINUATIONS
				if (cont)
					cont();
#endif
				cond.notify_all();
			}

			detail::atomic_counter refcnt;
			mutex mtx;
			condition_variable cond;
#if MIN11_HAS_FUTURE_CONTINUATIONS
			std::function<void()> cont;
#endif
			bool retrieved;
			volatile bool ready;
		};

		template<typename T>
		class promise_holder
		{
		public:
			explicit promise_holder(future_state<T>* st)
				: state(st)
			{
				st->add_ref();
			}

			promise_holder(const promise_holder& other)
				: state(other.state)
			{
				state->add_ref();
			}

			promise_holder& operator=(const promise_holder& other)
			{
				other.state->add_ref();
				state->dec_ref();
				state = other.state();
			}

			~promise_holder()
			{
				state->dec_ref();
			}

			promise<T> promise() const;

		private:
			detail::future_state<T>* state;
		};

		template<typename T, typename Data>
		class promise_holder_data
		{
		public:
			explicit promise_holder_data(future_state_data<T, Data>* st)
				: state(st)
			{
				st->add_ref();
			}

			promise_holder_data(const promise_holder_data& other)
				: state(other.state)
			{
				state->add_ref();
			}

			promise_holder_data& operator=(const promise_holder_data& other)
			{
				other.state->add_ref();
				state->dec_ref();
				state = other.state();
			}

			~promise_holder_data()
			{
				state->dec_ref();
			}

			promise_with_data<T, Data> promise() const;

		private:
			detail::future_state_data<T, Data>* state;
		};
	}

	template<>
	class future<void>
	{
		friend class promise<void>;
		friend class shared_future<void>;
		friend detail::make_ready_helper;
#if !MIN11_HAS_RVALREFS
		friend class detail::movable_future<void>;
#endif
	public:
		future()
			: state(0)
		{
		}

#if MIN11_HAS_RVALREFS
		future(future<void>&& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}

#else
		future(const detail::movable_future<void>& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}
#endif

		~future()
		{
			if (state)
				state->dec_ref();
		}

#if MIN11_HAS_RVALREFS
		future& operator=(future<void>&& rhs)
		{
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#else
		future& operator=(const detail::movable_future<void>& rhs)
		{
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#endif

		bool valid() const
		{
			return state && !state->retrieved;
		}

		void get()
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");
			state->get_value(false);
		}

		void wait() const
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");

			state->wait();
		}

		shared_future<void> share();

#if MIN11_HAS_FUTURE_CONTINUATIONS
		template<typename Func>
#if MIN11_HAS_RVALREFS
		void set_continuation(Func&& f)
#else
		void set_continuation(const Func& f)
#endif
		{
			if (!valid())
				detail::throw_future_error("future has no state object, likely shared");

#if MIN11_HAS_RVALREFS
			state->set_continuation(std::move(f));
#else
			state->set_continuation(f);
#endif
		}
#endif

	private:
		explicit future(const future&); // = delete;
		future& operator=(const future&); // = delete;

		future(detail::future_state<void>* s)
			: state(s)
		{
			if (s)
				s->add_ref();
		}

		detail::future_state<void>* state;
	};

	/**
	 * Minimal emulation for std::shared_future
	 */
	template<>
	class shared_future<void>
	{
	public:
		shared_future()
			: state(0)
		{
		}

#if MIN11_HAS_RVALREFS
		shared_future(future<void>&& rhs)
			: state(0)
		{
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
		}

		shared_future(shared_future<void>&& rhs)
			: state(rhs.state)
		{
			rhs.state = 0;
		}
#else
		shared_future(const detail::movable_future<void>& rhs)
			: state(0)
		{
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
		}
#endif

		shared_future(const shared_future<void>& other)
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

		shared_future& operator=(const shared_future& rhs)
		{
			if (rhs.state)
				rhs.state->add_ref();
			if (state)
				state->dec_ref();
			state = rhs.state;
			return *this;
		}

#if MIN11_HAS_RVALREFS
		shared_future& operator=(future<void>&& rhs)
		{
			if (state)
				state->dec_ref();

			state = 0;
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}

		shared_future& operator=(shared_future<void>&& rhs)
		{
			if (state)
				state->dec_ref();

			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#else
		shared_future& operator=(const detail::movable_future<void>& rhs)
		{
			if (state)
				state->dec_ref();

			state = 0;
#if MIN11_HAS_FUTURE_CONTINUATIONS
			if (rhs.state && rhs.state->cont)
				detail::throw_future_error("future with continuation cannot be shared");
#endif
			state = rhs.state;
			rhs.state = 0;
			return *this;
		}
#endif

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

		void get() const
		{
			if (!valid())
				detail::throw_future_error("future has no state object");

			state->get_value(true);
		}

	private:
		detail::future_state<void>* state;
	};

	/**
	 * Minimal emulation of std::promise
	 */
	template<>
	class promise<void>
	{
		friend detail::promise_holder<void>;
	public:
		promise()
			: state(new detail::future_state<void>)
		{
		}

#if MIN11_HAS_RVALREFS
		promise(promise&& other)
			: state(other.state)
		{
			other.state = 0;
		}
#endif

		~promise()
		{
			state->dec_ref();
		}

		void set_value()
		{
			state->set_value();
		}

#if MIN11_HAS_RVALREFS
		future<void> get_future()
		{
			return future<void>(state);
		}
#else
		detail::movable_future<void> get_future()
		{
			return detail::movable_future<void>(state);
		}
#endif

		detail::promise_holder<void> hold() const
		{
			return detail::promise_holder<void>(state);
		}

	private:
		promise(const promise&); // = delete;
		promise& operator=(const promise&); // = delete;

		promise(detail::future_state<void>* st)
			: state(st)
		{
			state->add_ref();
		}

		detail::future_state<void>* state;
	};

	inline shared_future<void> future<void>::share()
	{
		return shared_future<void>(move(*this));
	}

	namespace detail
	{
		struct make_ready_helper
		{
#if MIN11_HAS_RVALREFS
			template<typename T>
			static inline future<T> make_ready(T&& v)
			{
				detail::future_state<T>* st = new detail::future_state<T>;
				st->set_value(move(v));
				return future<T>(st);
			}
#endif // MIN11_HAS_RVALREFS

			template<typename T>
			static inline future<T> make_ready(const T& v)
			{
				detail::future_state<T>* st = new detail::future_state<T>;
				st->set_value(v);
				return future<T>(st);
			}

			static inline future<void> make_ready()
			{
				detail::future_state<void>* st = new detail::future_state<void>;
				st->set_value();
				return future<void>(st);
			}
		};
	} // namespace detail

#if MIN11_HAS_RVALREFS
	template<typename T>
	inline future<T> make_ready_future(T&& v)
	{
		return detail::make_ready_helper::make_ready(move(v));
	}
#endif // MIN11_HAS_RVALREFS

	template<typename T>
	inline future<T> make_ready_future(const T& v)
	{
		return detail::make_ready_helper::make_ready(v);
	}

	inline future<void> make_ready_future()
	{
		return detail::make_ready_helper::make_ready();
	}

	namespace detail
	{
		template<typename T>
		inline promise<T> promise_holder<T>::promise() const
		{
			return min11::promise<T>(state);
		}

		template<typename T, typename Data>
		inline promise_with_data<T, Data> promise_holder_data<T, Data>::promise() const
		{
			return min11::promise_with_data<T, Data>(state);
		}
	} // namespace detail
}

#endif // MIN11__FUTURE_H

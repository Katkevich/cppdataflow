#pragma once
#include <atomic>

namespace cppdf::details
{

	template<class T>
	class movable_atomic
	{
	public:
		movable_atomic()
		{
			//Do nothing.
		}
		template<class TVal>
		movable_atomic(TVal&& val)
		{
			inst_.store(std::forward<TVal>(val));
		}
		movable_atomic(movable_atomic&& that)
		{
			inst_.store(that.inst_.load());
		}
		movable_atomic& operator=(movable_atomic&& that)
		{
			inst_.store(that.inst_.load());
			return *this;
		}
		movable_atomic(const movable_atomic&) = delete;
		movable_atomic& operator=(const movable_atomic&) = delete;

		std::atomic<T>* operator->()
		{
			return &inst_;
		}

		const std::atomic<T>* operator->() const
		{
			return &inst_;
		}

		std::atomic<T>& inst()
		{
			return inst_;
		}

		const std::atomic<T>& inst() const
		{
			return inst_;
		}

	private:
		std::atomic<T> inst_;
	};

}
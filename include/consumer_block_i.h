#pragma once
#include <block_i.h>

namespace cppdf
{
	template<class T>
	class producer_block_i;

	template<class T>
	class consumer_block_i : public virtual block_i
	{
	public:
		virtual bool try_push(T&) = 0;
		virtual void register_producer(producer_block_i<T>&) = 0;
	};

}
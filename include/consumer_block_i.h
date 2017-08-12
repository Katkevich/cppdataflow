#pragma once
#include <memory>
#include <block_i.h>
#include <link.h>

namespace cppdf
{
	template<class T>
	class producer_block_i;

	template<class T>
	class consumer_block_i : public virtual block_i
	{
		template<class> friend class link;

	public:
		virtual bool try_push(T&) = 0;

	protected:
		virtual void connect_producer(const link<T>&) = 0;
	};

}
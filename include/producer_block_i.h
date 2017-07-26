#pragma once
#include <optional>
#include <block_i.h>

namespace cppdf
{
	template<class T>
	class consumer_block_i;

	template<class T>
	class producer_block_i : public virtual block_i
	{
	public:
		virtual std::optional<T> try_pull() = 0;
		virtual void link_to(consumer_block_i<T>&) = 0;
		virtual void register_consumer(consumer_block_i<T>&) = 0;
	};
}
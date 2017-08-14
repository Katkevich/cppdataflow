#pragma once
#include <optional>
#include <block_i.h>
#include <link.h>
#include <signals\connection.h>

namespace cppdf
{
	template<class T>
	class consumer_block_i;

	template<class T>
	class producer_block_i : public virtual block_i
	{
		template <class> friend class link;

	public:
		virtual std::optional<T> try_pull() = 0;
		virtual link<T> link_to(consumer_block_i<T>&) = 0;
		virtual signals::connection register_has_item_handler(std::function<void()>) = 0;

	protected:
		virtual void connect_consumer(const link<T>&) = 0;
	};
}
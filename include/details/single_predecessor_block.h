#pragma once
#include <concurrent_queue.h>

#include <details\completable_block.h>
#include <details\reservable_block.h>
#include <details\movable_atomic.h>
#include <producer_block_i.h>
#include <consumer_block_i.h>

namespace cppdf::details
{
	template<class TOut>
	class single_predecessor_block
		: public virtual details::completable_block
		, public virtual details::reservable_block
		, public producer_block_i<TOut>
	{
	public:
		single_predecessor_block()
		{
		}

		virtual link<TOut> link_to(consumer_block_i<TOut>& consumer) override
		{
			return link<TOut>(this, &consumer);
		}

	protected:
		virtual void connect_consumer(const link<TOut>& link) override
		{
			link_ = link;
		}

		std::optional<TOut> try_pull_impl()
		{
			std::optional<TOut> pulled_item;
			TOut item;
			auto success = queue_try_pop(item);
			if (success)
			{
				try_release();
				pulled_item = std::move(item);
			}
		
			return pulled_item;
		}

		void queue_push(TOut&& item)
		{
			items_.push(std::move(item));
		}

		bool queue_try_pop(TOut& item)
		{
			return items_.try_pop(item);
		}

		consumer_block_i<TOut>* get_consumer() const
		{
			return link_.get_consumer();
		}

	private:
		concurrency::concurrent_queue<TOut> items_;
		link<TOut> link_;
	};
}
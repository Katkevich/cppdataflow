#pragma once
#include <concurrent_queue.h>

#include <details\completable_block.h>
#include <details\reservable_block.h>
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
			consumer_.store(nullptr);
		}

		virtual void register_consumer(consumer_block_i<TOut>& consumer) override
		{
			consumer_.store(&consumer);
		}

		virtual void link_to(consumer_block_i<TOut>& consumer) override
		{
			register_consumer(consumer);
			consumer.register_producer(*this);
		}

	protected:
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
			return consumer_.load();
		}

	private:
		concurrency::concurrent_queue<TOut> items_;
		std::atomic<consumer_block_i<TOut>*> consumer_;
	};
}
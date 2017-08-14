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

		virtual std::optional<TOut> try_pull() override
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


		virtual link<TOut> link_to(consumer_block_i<TOut>& consumer) override
		{
			return link<TOut>(this, &consumer);
		}

		virtual signals::connection register_has_item_handler(std::function<void()> handler)
		{
			return has_items_signal_.connect(std::move(handler));
		}

	protected:
		virtual void connect_consumer(const link<TOut>& link) override
		{
			link_ = link;
		}

		void queue_push(TOut&& item)
		{
			items_.push(std::move(item));
			has_items_signal_();
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
		signals::signal<void(void)> has_items_signal_;
		link<TOut> link_;
	};
}
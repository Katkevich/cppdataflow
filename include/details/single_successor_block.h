#pragma once
#include <concurrent_queue.h>

#include <details\completable_block.h>
#include <details\reservable_block.h>
#include <details\movable_atomic.h>
#include <producer_block_i.h>
#include <consumer_block_i.h>

namespace cppdf::details
{
	template<class TIn>
	class single_successor_block
		: public virtual details::completable_block
		, public virtual details::reservable_block
		, public consumer_block_i<TIn>
	{
	public:
		single_successor_block()
		{
		}

	protected:
		virtual void connect_producer(const link<TIn>& link) override
		{
			link_ = link;
			link_.on_producer_completion([this] { producer_done(); });
		}

		bool try_push_impl(TIn& item)
		{
			if (is_completion())
				return false;

			auto has_slot = try_reserve();
			if (has_slot)
				process_item(std::move(item));

			return has_slot;
		}

		void pull_push()
		{
			producer_block_i<TIn>* producer = nullptr;

			while ((producer = get_producer()) && try_reserve())
			{
				auto produced_item = producer->try_pull();
				if (produced_item.has_value())
					process_item(std::move(produced_item.value()));
				else
				{
					try_release();
					break;
				}
			}
		}

		producer_block_i<TIn>* get_producer() const
		{
			return link_.get_producer();
		}

		virtual void process_item(TIn&&) = 0;
		virtual void producer_done() = 0;

	private:
		std::optional<signals::connection> producer_completion_connection_;
		link<TIn> link_;
	};
}
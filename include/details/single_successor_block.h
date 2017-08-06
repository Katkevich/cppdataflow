#pragma once
#include <concurrent_queue.h>

#include <details\completable_block.h>
#include <details\reservable_block.h>
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
			producer_.store(nullptr);
		}

		virtual void register_producer(producer_block_i<TIn>& producer) override
		{
			if (producer_completion_connection_.has_value())
				producer_completion_connection_->disconnect();

			producer_.store(&producer);
			producer_completion_connection_ = producer.register_completion_handler([this] { producer_done(); });
		}

	protected:
		bool try_push_impl(TIn& item)
		{
			if (is_completion())
				return false;

			auto has_slot = try_reserve();
			if (has_slot)
				process_item(std::move(item));

			return has_slot;
		}

		void producer_pull_push()
		{
			auto producer = producer_.load();
			if (producer && try_reserve())
			{
				auto produced_item = producer->try_pull();
				if (produced_item.has_value())
					process_item(std::move(produced_item.value()));
				else
					try_release();
			}
		}

		producer_block_i<TIn>* get_producer() const
		{
			return producer_.load();
		}

		virtual void process_item(TIn&&) = 0;
		virtual void producer_done() = 0;

	private:
		std::optional<signals::connection> producer_completion_connection_;
		std::atomic<producer_block_i<TIn>*> producer_;
	};
}
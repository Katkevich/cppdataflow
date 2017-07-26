#pragma once
#include <concurrent_queue.h>

#include <consumer_block_i.h>
#include <producer_block_i.h>
#include <details\block_base.h>
#include <signals\connection.h>

namespace cppdf
{

	template<class T>
	class buffer_block
		: public details::block_base
		, public consumer_block_i<T>
		, public producer_block_i<T>
	{
	public:
		buffer_block(unsigned short capacity)
			: capacity_((int)capacity)
			, producer_(nullptr)
			, consumer_(nullptr)
		{
			load_factor_.store(0);
		}

		virtual bool try_push(T& item) override
		{
			if (is_completion_.load())
				return false;

			auto has_slot = try_reserve_slot();
			if (has_slot)
				process_item(std::move(item));

			return has_slot;
		}

		virtual std::optional<T> try_pull() override
		{
			std::optional<T> pulled_item;
			T item;
			auto success = buffered_items_.try_pop(item);
			if (success)
			{
				try_release_slot();
				pulled_item = std::move(item);
			}

			auto producer_safe_ptr = producer_;
			if (producer_safe_ptr && try_reserve_slot())
			{
				auto produced_item = producer_safe_ptr->try_pull();
				if (produced_item.has_value())
					process_item(std::move(produced_item.value()));
				else
					try_release_slot();
			}

			try_make_completed();

			return pulled_item;
		}


		virtual void register_producer(producer_block_i<T>& producer) override
		{
			if (producer_completion_connection_.has_value())
				producer_completion_connection_->disconnect();

			producer_ = &producer;
			producer_completion_connection_ = producer.register_completion_handler([this] { try_make_completed(); });
		}

		virtual void register_consumer(consumer_block_i<T>& consumer) override
		{
			consumer_ = &consumer;
		}

		virtual void link_to(consumer_block_i<T>& consumer) override
		{
			register_consumer(consumer);
			consumer.register_producer(*this);
		}

	private:
		void try_make_completed()
		{
			auto producer_safe_ptr = producer_;
			auto is_completed = producer_safe_ptr && producer_safe_ptr->is_done() && load_factor_.load() == 0;
			if (is_completed && !is_done())
			{
				complete();
				completion_promise_.set_value();
				completion_signal_();
			}
		}

		bool try_reserve_slot()
		{
			if (load_factor_.fetch_add(1) >= capacity_)
			{
				load_factor_.fetch_sub(1);
				return false;
			}

			return true;
		}

		bool try_release_slot()
		{
			if (load_factor_.fetch_sub(1) < 0)
			{
				load_factor_.fetch_add(1);
				return false;
			}

			return true;
		}

		void process_item(T&& item)
		{
			auto consumer_safe_ptr = consumer_;
			if (consumer_safe_ptr && consumer_safe_ptr->try_push(item))
				try_release_slot();
			else
				buffered_items_.push(std::move(item));
		}

	private:
		std::optional<signals::connection> producer_completion_connection_;
		concurrency::concurrent_queue<T> buffered_items_;

		int capacity_;
		std::atomic<int> load_factor_;

		producer_block_i<T>* producer_;
		consumer_block_i<T>* consumer_;
	};

}
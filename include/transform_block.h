#pragma once
#include <functional>
#include <memory>
#include <future>
#include <ppl.h>
#include <ppltasks.h>
#include <concurrent_queue.h>
#include <concurrent_vector.h>

#include <producer_block_i.h>
#include <consumer_block_i.h>
#include <signals\signal.h>
#include <details\block_base.h>

namespace cppdf
{

	template<class TFrom, class TTo>
	class transform_block
		: public details::block_base
		, public consumer_block_i<TFrom>
		, public producer_block_i<TTo>
	{
	public:
		template<class Fn>
		transform_block(unsigned short parallelism, Fn&& body)
			: body_(std::forward<Fn>(body))
			, parallelism_((int)parallelism)
			, producer_(nullptr)
			, consumer_(nullptr)
		{
			load_factor_.store(0);
			loop_next_step_notified_.store(false);
			run_pipeline_loop();
		}
		~transform_block() = default;

		virtual bool try_push(TFrom& item) override
		{
			if (is_completion_.load())
				return false;

			auto has_slot = try_reserve_slot();
			if (has_slot)
				process_item(std::move(item));

			return has_slot;
		}

		virtual std::optional<TTo> try_pull() override
		{
			TTo item;
			auto success = complete_items_.try_pop(item);
			if (success)
			{
				try_release_slot();
				notify_loop();

				return std::move(item);
			}
			else if (producer_ && try_reserve_slot())
			{
				auto from_item = producer_->try_pull();
				if (from_item.has_value())
					process_item(std::move(from_item.value()));
				else
					try_release_slot();
			}

			return std::optional<TTo>();
		}


		virtual void register_producer(producer_block_i<TFrom>& producer) override
		{
			if (producer_completion_connection_.has_value())
				producer_completion_connection_->disconnect();

			producer_ = &producer;
			producer_completion_connection_ = producer_->register_completion_handler([this] { notify_loop(); });
		}

		virtual void register_consumer(consumer_block_i<TTo>& consumer) override
		{
			consumer_ = &consumer;
		}

		virtual void link_to(consumer_block_i<TTo>& consumer) override
		{
			register_consumer(consumer);
			consumer_->register_producer(*this);
		}

		virtual void complete() override
		{
			details::block_base::complete();
			notify_loop();
		}

	private:
		bool try_reserve_slot()
		{
			if (load_factor_.fetch_add(1) >= parallelism_)
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

		void notify_loop()
		{
			std::unique_lock<std::mutex> lock(loop_mutex_);
			loop_next_step_notified_.store(true);
			loop_next_step_cv_.notify_one();
		}

		void wait_loop_notification()
		{
			std::unique_lock<std::mutex> lock(loop_mutex_);
			loop_next_step_cv_.wait(lock, [this] { return loop_next_step_notified_.load(); });
			loop_next_step_notified_.store(false);
		}

		void process_item(TFrom&& item)
		{
			concurrency::create_task([this, item = std::move(item)]{
				auto result = body_(std::move(item));
				complete_items_.push(std::move(result));

				notify_loop();
			});
		}

		void run_pipeline_loop()
		{
			concurrency::create_task([this] {
				while (!(load_factor_.load() == 0 && (is_completion_.load() || producer_ && producer_->is_done())))
				{
					wait_loop_notification();

					TTo item;
					while (consumer_ && complete_items_.try_pop(item))
					{
						if (consumer_->try_push(item))
							try_release_slot();
						else
						{
							complete_items_.push(std::move(item));
							break;
						}
					}
				}

				if (producer_)
					producer_->get_completion().wait();

				completion_promise_.set_value();
				completion_signal_();
			});
		}

	private:
		std::mutex loop_mutex_;
		std::condition_variable loop_next_step_cv_;
		std::atomic<bool> loop_next_step_notified_;

		std::function<TTo(TFrom)> body_;
		signed int parallelism_;

		std::optional<signals::connection> producer_completion_connection_;

		producer_block_i<TFrom>* producer_;
		consumer_block_i<TTo>* consumer_;

		concurrency::concurrent_queue<TTo> complete_items_;
		std::atomic<signed int> load_factor_;
	};

}


#pragma once
#include <memory>
#include <functional>
#include <atomic>

#include <consumer_block_i.h>
#include <producer_block_i.h>
#include <signals\signal.h>
#include <details\block_base.h>

namespace cppdf
{

	template<class T>
	class action_block 
		: public details::block_base
		, public consumer_block_i<T>
	{
	public:
		template<class Fn>
		action_block(unsigned short parallelism, Fn&& body)
			: body_(std::forward<Fn>(body))
			, parallelism_((int)parallelism)
			, producer_(nullptr)
		{
			load_factor_.store(0);
			loop_next_step_notified_.store(false);
			run_pipeline_loop();
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

		virtual void register_producer(producer_block_i<T>& producer) override
		{
			if (producer_completion_connection_.has_value())
				producer_completion_connection_->disconnect();

			producer_ = &producer;
			producer_completion_connection_ = producer_->register_completion_handler([this] { notify_loop(); });
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
			if (load_factor_.fetch_sub(1) == 0)
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

		void process_item(T&& item)
		{
			concurrency::create_task([this, item = std::move(item)]{
				body_(std::move(item));

				notify_loop();
			});
		}

		void run_pipeline_loop()
		{
			concurrency::create_task([this] {
				while(!(load_factor_.load() == 0 && (is_completion_.load() || producer_ && producer_->is_done())))
				{
					wait_loop_notification();

					std::optional<T> item;
					if (producer_)
						item = producer_->try_pull();

					if (item.has_value())
						process_item(std::move(item.value()));
					else
						try_release_slot();
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


		std::function<void(T)> body_;

		int parallelism_;
		std::atomic<int> load_factor_;

		producer_block_i<T>* producer_;
		std::optional<signals::connection> producer_completion_connection_;
	};

}
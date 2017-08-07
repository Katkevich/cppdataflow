#pragma once
#include <memory>
#include <functional>
#include <atomic>
#include <concurrent_queue.h>

#include <details\single_successor_block.h>
#include <details\complete_msg.h>
#include <details\loop_manager.h>
#include <details\block_body.h>

namespace cppdf
{

	template<class TIn>
	class action_block 
		: public details::single_successor_block<TIn>
	{
	public:
		template<class Fn>
		action_block(unsigned short parallelism, Fn&& body) noexcept
			: details::reservable_block(parallelism)
			, body_(std::function<decltype(body(std::declval<TIn>()))(TIn)>(std::forward<Fn>(body)))
		{
			run_pipeline_loop();
		}

		virtual bool try_push(TIn& item) override
		{
			return try_push_impl(item);
		}

		virtual void complete() override
		{
			details::completable_block::complete();
			loop_mngr_.notify();
		}

	private:
		virtual void process_item(TIn&& item) override
		{
			concurrency::create_task([this, item = std::move(item)]{
				body_.invoke(std::move(item));
				complete_msgs_.push(details::complete_msg{});

				loop_mngr_.notify();
			});
		}

		virtual void producer_done() override
		{
			loop_mngr_.notify();
		}

		void run_pipeline_loop()
		{
			concurrency::create_task([this] {
				producer_block_i<TIn>* producer = nullptr;

				while (!(is_empty() && (is_completion() || (producer = get_producer()) && producer->is_done())))
				{
					loop_mngr_.wait_notification();

					details::complete_msg complete_msg;
					while (complete_msgs_.try_pop(complete_msg))
					{
						std::optional<TIn> item;
						if (producer = get_producer())
							item = producer->try_pull();
						
						if (item.has_value())
							process_item(std::move(item.value()));
						else
							try_release();
					}
				}

				if (producer = get_producer())
					producer->get_completion().wait();

				finish();
			});
		}

	private:
		details::loop_manager loop_mngr_;
		details::block_body<TIn> body_;
		concurrency::concurrent_queue<details::complete_msg> complete_msgs_;
	};

}
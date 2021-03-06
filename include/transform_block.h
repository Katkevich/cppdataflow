#pragma once
#include <functional>
#include <memory>
#include <future>
#include <ppl.h>
#include <ppltasks.h>
#include <concurrent_queue.h>
#include <details\reservable_block.h>

#include <details\block_body.h>
#include <details\loop_manager.h>
#include <details\single_successor_block.h>
#include <details\single_predecessor_block.h>

namespace cppdf
{

	template<class TIn, class TOut>
	class transform_block
		: public details::single_successor_block<TIn>
		, public details::single_predecessor_block<TOut>
	{
	public:
		template<class Fn>
		transform_block(unsigned short parallelism, Fn&& body)
			: details::reservable_block(parallelism)
			, body_(std::forward<Fn>(body))
		{
			run_pipeline_loop();
		}

		virtual std::optional<TOut> try_pull() override
		{
			auto item = details::single_predecessor_block<TOut>::try_pull();
			loop_mngr_.notify();

			return item;
		}

		virtual void complete() override
		{
			details::completable_block::complete();
			loop_mngr_.notify();
		}

	private:
		virtual void on_producer_done() override
		{
			loop_mngr_.notify();
		}

		virtual void process_item(TIn&& item) override
		{
			concurrency::create_task([this, item = std::move(item)]{
				auto result = body_.invoke(std::move(item));

				queue_push(std::move(result));
				loop_mngr_.notify();
			});
		}

		void run_pipeline_loop()
		{
			concurrency::create_task([this] {
				producer_block_i<TIn>* producer = nullptr;

				while (!(is_empty() && (is_completion() || (producer = get_producer()) && producer->is_done())))
				{
					loop_mngr_.wait_notification();
					pull_push();
				}

				if (producer = get_producer())
					producer->get_completion().wait();

				finish();
			});
		}

	private:
		details::loop_manager loop_mngr_;
		details::block_body<TIn, TOut> body_;
	};

}


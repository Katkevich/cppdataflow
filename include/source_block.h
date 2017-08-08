#pragma once
#include <optional>
#include <details\completable_block.h>
#include <details\single_predecessor_block.h>
#include <details\block_body.h>
#include <details\loop_manager.h>
#include <producer_block_i.h>

namespace cppdf
{
	template<class TOut>
	class source_block 
		: public details::single_predecessor_block<TOut>
	{
	public:
		template<class Fn>
		source_block(unsigned short capacity, Fn&& body)
			: details::reservable_block(capacity)
			, body_(std::forward<Fn>(body))
		{
			run_pipeline_loop();
		}

		virtual std::optional<TOut> try_pull() override
		{
			auto item = try_pull_impl();

			loop_mngr_.notify();

			return item;
		}

		virtual void complete() override
		{
			details::completable_block::complete();
			loop_mngr_.notify();
		}

	private:
		void run_pipeline_loop()
		{
			concurrency::create_task([this] {
				std::optional<TOut> item;
				do
				{
					loop_mngr_.wait_notification();

					while (try_reserve())
					{
						item = body_.invoke();
						if (item)
						{
							auto consumer = get_consumer();
							if (consumer && consumer->try_push(item.value()))
								try_release();
							else
								queue_push(std::move(item.value()));
						}
						else
							break;
					}
				} while (!is_completion() && item.has_value());

				finish();
			});
		}

	private:
		details::loop_manager loop_mngr_;
		details::block_body<void, std::optional<TOut>> body_;
	};
}
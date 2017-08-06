#pragma once
#include <atomic>
#include <future>
#include <optional>

#include <block_i.h>
#include <signals\signal.h>
#include <signals\connection.h>

namespace cppdf::details
{

	class completable_block : public virtual block_i
	{
	public:
		completable_block()
		{
			completion_future_ = completion_promise_.get_future();
			is_completion_.store(false);
		}

		virtual const std::future<void>& get_completion() const override
		{
			return completion_future_;
		}

		virtual signals::connection register_completion_handler(std::function<void()> handler) override
		{
			return completion_signal_.connect(std::move(handler));
		}

		virtual void complete() override
		{
			is_completion_.store(true);
		}

		virtual bool is_done() const override
		{
			auto result = 
				completion_future_.valid() && 
				completion_future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
			return result;
		}

	protected:
		bool is_completion() const
		{
			return is_completion_.load();
		}

		void finish()
		{
			completion_promise_.set_value();
			completion_signal_();
		}

	private:
		std::future<void> completion_future_;
		std::promise<void> completion_promise_;
		signals::signal<void(void)> completion_signal_;
		std::atomic<bool> is_completion_;
	};

}
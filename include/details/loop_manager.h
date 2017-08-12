#pragma once
#include <mutex>
#include <atomic>
#include <details\movable_atomic.h>

namespace cppdf::details
{
	class loop_manager
	{
	public:
		loop_manager()
			: loop_next_step_notified_(true)
		{
		}

		void notify()
		{
			std::unique_lock<std::mutex> lock(loop_mutex_);
			loop_next_step_notified_->store(true);
			loop_next_step_cv_.notify_one();
		}

		void wait_notification()
		{
			std::unique_lock<std::mutex> lock(loop_mutex_);
			loop_next_step_cv_.wait(lock, [this] { return loop_next_step_notified_->load(); });
			loop_next_step_notified_->store(false);
		}

	private:
		std::mutex loop_mutex_;
		std::condition_variable loop_next_step_cv_;
		details::movable_atomic<bool> loop_next_step_notified_;
	};
}
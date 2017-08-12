#pragma once
#include <details\movable_atomic.h>

namespace cppdf::details
{
	class reservable_block
	{
	protected:
		reservable_block(unsigned short capacity)
			: capacity_((int)capacity)
		{
			load_factor_->store(0);
		}

		bool is_empty() const
		{
			return load_factor_->load() <= 0;
		}

		bool try_reserve()
		{
			if (load_factor_->fetch_add(1) > capacity_)
			{
				load_factor_->fetch_sub(1);
				return false;
			}

			return true;
		}

		bool try_release()
		{
			if (load_factor_->fetch_sub(1) == 0)
			{
				load_factor_->fetch_add(1);
				return false;
			}

			return true;
		}

	private:
		details::movable_atomic<signed int> load_factor_;
		signed int capacity_;
	};
}
#pragma once
#include <atomic>
#include "signal_base.h"

namespace cppdf::signals
{
	class connection
	{
	public:
		struct hasher
		{
			std::size_t operator()(const connection &p) const 
			{
				return std::hash<std::size_t>{}(p.id_);
			}
		};

		explicit connection(details::signal_base& signal)
			: id_(generate_id())
			, signal_(&signal)
		{
			//do nothing.
		}

		void disconnect()
		{
			signal_->disconnect(*this);
		}

		bool operator==(const connection& that) const
		{
			return id_ == that.id_;
		}

	private:
		std::size_t generate_id() const
		{
			static int id;
			id++;
			return id;
		}

	private:
		std::size_t id_;
		details::signal_base* signal_;
	};
}
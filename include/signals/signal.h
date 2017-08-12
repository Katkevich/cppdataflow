#pragma once
#include <functional>
#include <cassert>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <signals\connection.h>
#include <details\movable_atomic.h>

namespace cppdf::signals
{
	template<class TFun>
	class signal : public details::signal_base
	{
	public:
		template<class UFun>
		connection connect(UFun fun)
		{
			auto found_it = std::find_if(slots_.begin(), slots_.end(), [](auto& pair) { return !pair.first.is_active->load(); });
			if (found_it != slots_.end())
			{
				found_it->second = std::move(fun);
				const_cast<cppdf::details::movable_atomic<bool>&>(found_it->first.is_active)->store(true);

				return found_it->first.conn;
			}
			else
			{
				connection conn(*this);
				slots_.insert(std::pair<connection_data, std::function<TFun>>({ conn }, std::move(fun)));

				return conn;
			}
		}

		template<class ...TArgs>
		void operator()(TArgs ...args)
		{
			for (auto& pair : slots_)
			{
				if (pair.first.is_active->load())
					pair.second(args...);
			}
		}

	protected:
		virtual void disconnect(connection& conn) override
		{
			connection_data key{ conn };
			auto found_it = slots_.find(key);

			assert(found_it != slots_.end());

			const_cast<cppdf::details::movable_atomic<bool>&>(found_it->first.is_active)->store(false);
		}

	private:
		struct connection_data
		{
			struct hasher
			{
				std::size_t operator()(const connection_data &c) const
				{
					return connection::hasher{}(c.conn);
				}
			};

			connection_data(connection& conn)
				: conn(conn)
			{
				is_active->store(true);
			}

			connection_data(const connection_data& that)
				: conn(that.conn)
			{
				is_active->store(that.is_active->load());
			}

			bool operator==(const connection_data& that) const
			{
				return conn == that.conn;
			}

			connection conn;
			cppdf::details::movable_atomic<bool> is_active;
		};

	private:
		concurrency::concurrent_unordered_map<connection_data, std::function<TFun>, typename connection_data::hasher> slots_;
	};
}
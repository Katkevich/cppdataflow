#pragma once
namespace cppdf::signals
{
	class connection;
}

namespace cppdf::signals::details
{
	class signal_base
	{
	public:
		virtual ~signal_base() = default;

		virtual void disconnect(connection&) = 0;
	};
}
#pragma once
#include <future>
#include <functional>
#include <signals\connection.h>

namespace cppdf
{

	class block_i
	{
	public:
		virtual ~block_i() = default;

		virtual const std::future<void>& get_completion() const = 0;
		virtual signals::connection register_completion_handler(std::function<void()>) = 0;
		virtual void complete() = 0;
		virtual bool is_done() const = 0;
	};

}
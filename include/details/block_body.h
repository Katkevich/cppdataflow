#pragma once
#include <functional>
#include <variant>
#include <future>

#if __has_include(<ppltasks.h>)
#	include <ppltasks.h>
#	define HAS_PPL 1
namespace cppdf {
	namespace ppl = concurrency;
}

#elif __has_include(<pplx\pplxtasks.h>)
#	include <pplx/pplxtasks.h>
#	define HAS_PPLX 1
namespace cppdf {
	namespace ppl = pplx;
}

#endif


namespace cppdf::details
{
	template<class TIn, class TOut = void>
	class block_body
	{
	public:
		template<class Fn>
		block_body(Fn&& body)
			: body_(std::forward<Fn>(body))
		{
			//Do nothing.
		}

		TOut invoke(TIn in)
		{
			struct void_return {};

			auto result = std::visit([&](auto&& body) {
				using item_t = std::decay_t<decltype(body)>::result_type;

				if constexpr (std::is_same_v<TOut, void>)
				{
					if constexpr (std::is_same_v<item_t, TOut>)
						body(std::move(in));
					else
						body(std::move(in)).wait();

					return void_return{};
				}
				else
					if constexpr (std::is_same_v<item_t, TOut>)
						return body(std::move(in));
					else
						return body(std::move(in)).get();
			}, body_);

			if constexpr (!std::is_same_v<decltype(result), void_return>)
				return result;
		}

	private:
		std::variant<
#if defined(HAS_PPL) || defined(HAS_PPLX)
			std::function<ppl::task<TOut>(TIn)>,
#endif
			std::function<std::future<TOut>(TIn)>,
			std::function<TOut(TIn)>
		> body_;
	};
}
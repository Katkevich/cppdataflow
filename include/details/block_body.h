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

		block_body(block_body&&) = default;
		block_body& operator=(block_body&&) = default;
		block_body(const block_body&) = delete;
		block_body& operator=(const block_body&) = delete;

		template<class = std::enable_if_t<std::is_same_v<TIn, void>>>
		TOut invoke()
		{
			if constexpr (std::is_same_v<TOut, void>)
				invoke(void_stub{});
			else
				return invoke(void_stub{});
		}

		template<class UIn>
		TOut invoke(UIn&& in)
		{
			auto result = std::visit([&](auto&& body) {
				using ret_t = std::decay_t<decltype(body)>::result_type;
				using par_t = std::decay_t<UIn>;

				if constexpr (std::is_same_v<TOut, void>)
				{
					if constexpr (std::is_same_v<par_t, void_stub>)
						if constexpr (std::is_same_v<ret_t, TOut>)
							body();
						else
							body().wait();
					else
						if constexpr (std::is_same_v<ret_t, TOut>)
							body(std::forward<UIn>(in));
						else
							body(std::forward<UIn>(in)).wait();

					return void_stub{};
				}
				else
					if constexpr (std::is_same_v<par_t, void_stub>)
						if constexpr (std::is_same_v<ret_t, TOut>)
							return body();
						else
							return body().get();
					else
						if constexpr (std::is_same_v<ret_t, TOut>)
							return body(std::forward<UIn>(in));
						else
							return body(std::forward<UIn>(in)).get();
			}, body_);

			if constexpr (!std::is_same_v<decltype(result), void_stub>)
				return result;
		}

	private:
		struct void_stub {};

		std::variant<
#if defined(HAS_PPL) || defined(HAS_PPLX)
			std::function<ppl::task<TOut>(TIn)>,
#endif
			std::function<std::future<TOut>(TIn)>,
			std::function<TOut(TIn)>
		> body_;
	};
}
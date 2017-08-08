#include <iostream>
#include <chrono>

#include <action_block.h>
#include <transform_block.h>
#include <buffer_block.h>
#include <source_block.h>

#include <ppltasks.h>

using namespace std;

int main()
{
	int sum = 0;

	int idx = 0;
	int max_idx = 7;
	cppdf::source_block<char> sb(4, [&] {
		if (idx++ <= max_idx)
			return std::make_optional((char)('0' + idx));
		return std::optional<char>();
	});
	cppdf::transform_block<char, wchar_t> tb(4, [](auto c) {
		return concurrency::create_task([c] { return (wchar_t)c; });
	});

	cppdf::buffer_block<wchar_t> bb(4);

	cppdf::action_block<wchar_t> ab(4, [](wchar_t c) {
		return std::async([c] { std::cout << (char)c; });
	});

	sb.link_to(tb);
	tb.link_to(bb);
	bb.link_to(ab);

	ab.get_completion().wait();

	std::cout << "\nDone." << std::endl;

	return 0;
}
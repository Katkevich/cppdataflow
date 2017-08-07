#include <iostream>
#include <chrono>

#include <action_block.h>
#include <transform_block.h>
#include <buffer_block.h>

#include <ppltasks.h>

using namespace std;

int main()
{
	int sum = 0;

	/*cppdf::transform_block<char, wchar_t> tb(4, [](auto c) {
		return std::async([&] { return (wchar_t)c; });
	});*/
	cppdf::transform_block<char, wchar_t> tb(4, [](auto c) {
		return concurrency::create_task([c] { return (wchar_t)c; });
	});

	cppdf::buffer_block<wchar_t> bb(4);

	cppdf::action_block<wchar_t> ab(4, [](wchar_t c) {
		return std::async([c] {
			std::cout << (char)c;
		});
	});

	tb.link_to(bb);
	bb.link_to(ab);

	char c1 = '1';
	char c2 = '2';
	char c3 = '3';
	char c4 = '4';
	char c5 = '5';
	char c6 = '6';
	char c7 = '7';
	char c8 = '8';
	tb.try_push(c1);
	tb.try_push(c2);
	tb.try_push(c3);
	tb.try_push(c4);
	std::this_thread::sleep_for(200ms);
	auto res5 = tb.try_push(c5);
	auto res6 = tb.try_push(c6);
	auto res7 = tb.try_push(c7);
	auto res8 = tb.try_push(c8);

	tb.complete();

	ab.get_completion().wait();

	std::cout << "Done." << std::endl;

	return 0;
}
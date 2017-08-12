#include <iostream>
#include <chrono>
#include <random>

#include <action_block.h>
#include <transform_block.h>
#include <buffer_block.h>
#include <source_block.h>

#include <ppltasks.h>

using namespace std;

int main()
{
	std::default_random_engine engine;
	std::uniform_int_distribution<int> dist(200, 700);
	

	int idx = 0;
	int max_idx = 25;
	cppdf::source_block<char> sb(4, [&] {
		if (idx++ <= max_idx)
			return std::make_optional((char)(0x60 + idx));
		return std::optional<char>();
	});
	cppdf::transform_block<char, wchar_t> tb(4, [&](auto c) {
		return concurrency::create_task([&engine, &dist, c] { 
			std::this_thread::sleep_for(std::chrono::milliseconds(dist(engine))); 
			return (wchar_t)c; 
		});
	});

	cppdf::buffer_block<wchar_t> bb(4);

	cppdf::action_block<wchar_t> ab(4, [&](wchar_t c) {
		return std::async([&bb, &dist, &engine, c] { 
			std::this_thread::sleep_for(std::chrono::milliseconds(2 * dist(engine)));
			std::cout << (char)c; 
		});
	});

	sb.link_to(tb);
	tb.link_to(bb);
	bb.link_to(ab);

	ab.get_completion().wait();
	
	std::cout << "\nDone." << std::endl;

	return 0;
}
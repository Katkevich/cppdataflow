#include <iostream>
#include <chrono>

#include <action_block.h>
#include <transform_block.h>
#include <buffer_block.h>

using namespace std;

class Foo
{
public:
	Foo()
	{
		std::cout << "ctor" << std::endl;
	}
	Foo(Foo&& f)
	{
		std::cout << "move ctor" << std::endl;
	}

	Foo& operator=(Foo&& f)
	{
		std::cout << "move op" << std::endl;
		return *this;
	}

	Foo(const Foo& foo)
	{

		std::cout << "copy ctor" << std::endl;
	}

	Foo& operator=(const Foo&)
	{
		std::cout << "copy op" << std::endl;
	}
};

void bar(Foo f)
{
	std::cout << "bar call" << std::endl;
}

void foo(Foo&& f)
{
	bar(f);
	auto l = [f = std::move(f)]() { std::cout << "lambda call" << std::endl; };
	l();
}

int main()
{
	Foo f;
	foo(std::move(f));

	int sum = 0;

	cppdf::transform_block<char, wchar_t> tb(1, [](auto c) {
		return (wchar_t)c;
	});
	cppdf::buffer_block<wchar_t> bb(1);
	cppdf::action_block<wchar_t> ab(1, [&sum](auto c) {
		std::cout << (char)c;
		sum += (int)c;
	});

	//tb.link_to(ab);
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
	auto res5 = tb.try_push(c5);
	auto res6 = tb.try_push(c6);
	auto res7 = tb.try_push(c7);
	auto res8 = tb.try_push(c8);

	tb.complete();

	ab.get_completion().wait();

	std::cout << "Done." << std::endl;

	return 0;
}
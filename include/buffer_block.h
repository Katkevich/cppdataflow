#pragma once
#include <concurrent_queue.h>

#include <consumer_block_i.h>
#include <producer_block_i.h>
#include <details\completable_block.h>
#include <signals\connection.h>

#include <details\single_predecessor_block.h>
#include <details\single_successor_block.h>

namespace cppdf
{

	template<class T>
	class buffer_block
		: public details::single_predecessor_block<T>
		, public details::single_successor_block<T>
	{
	public:
		buffer_block(unsigned short capacity)
			: details::reservable_block(capacity)
		{
			//Do nothing.
		}
	
		virtual bool try_push(T& item) override
		{
			return try_push_impl(item);
		}

		virtual std::optional<T> try_pull() override
		{
			auto item = try_pull_impl();
			pull_push();

			try_finish();

			return item;
		}

	private:
		void try_finish()
		{
			auto producer = get_producer();
			auto is_completed = is_empty() && producer && producer->is_done();

			if (is_completed && !is_done())
				finish();
		}

		virtual void producer_done() override
		{
			try_finish();
		}
	
		virtual void process_item(T&& item) override
		{
			auto consumer = get_consumer();
			if (consumer && consumer->try_push(item))
				try_release();
			else
				queue_push(std::move(item));
		}
	};

}
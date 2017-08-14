#pragma once
#include <optional>
#include <producer_block_i.h>
#include <consumer_block_i.h>
#include <signals\connection.h>

namespace cppdf
{
	template<class T>
	class link
	{
	public:
		link()
			: impl_(std::make_shared<link_impl>(nullptr, nullptr))
		{
		}

		link(producer_block_i<T>* producer, consumer_block_i<T>* consumer)
			: impl_(std::make_shared<link_impl>(producer, consumer))
		{
			producer->connect_consumer(*this);
			consumer->connect_producer(*this);
		}

		signals::connection on_producer_completion(std::function<void()> handler)
		{
			impl_->producer_completion_connection_ = 
				impl_->producer_.load()->register_completion_handler(std::move(handler));
			return impl_->producer_completion_connection_.value();
		}

		signals::connection on_producer_has_item(std::function<void()> handler)
		{
			return impl_->producer_.load()->register_has_item_handler(std::move(handler));
		}

		void destroy()
		{
			impl_->destroy();
		}

		producer_block_i<T>* get_producer() const
		{
			return impl_->producer_.load();
		}

		consumer_block_i<T>* get_consumer() const
		{
			return impl_->consumer_.load();
		}

	private:
		struct link_impl
		{
			std::atomic<producer_block_i<T>*> producer_;
			std::atomic<consumer_block_i<T>*> consumer_;
			std::optional<signals::connection> producer_completion_connection_;

			link_impl(producer_block_i<T>* producer, consumer_block_i<T>* consumer)
			{
				producer_.store(producer);
				consumer_.store(consumer);
			}
			~link_impl()
			{
				destroy();
			}

			void destroy()
			{
				if (producer_completion_connection_.has_value())
					producer_completion_connection_->disconnect();

				producer_.store(nullptr);
				consumer_.store(nullptr);
			}
		};

	private:
		std::shared_ptr<link_impl> impl_;
	};
}
#ifndef ELOG_ASYNC_LOGGER_HPP
#define ELOG_ASYNC_LOGGER_HPP

#include "elog/buffer.hpp"
#include "elog/file_manager.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <latch>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
namespace elog
{
namespace details
{
struct AsyncLogger
{
	inline constexpr static std::size_t kSmallBuffer = 4096;
	inline constexpr static std::size_t kLargeBuffer = 65536;

	using LoggerBuffer = Buffer<kLargeBuffer>;

	explicit AsyncLogger(
		const std::string& dir, const std::string& prefix,
		std::size_t roll_size = 100 * 1024 * 1024,
		std::chrono::seconds flush_interval = std::chrono::seconds(3),
		std::size_t check_per_count = 1024)
	{
		std::latch start_latch{1};
		thread_ = std::jthread(
			[&] {
				run(dir, prefix, roll_size, flush_interval, check_per_count,
					start_latch);
			});
		start_latch.wait();
	}

	AsyncLogger(AsyncLogger&&) = delete;
	~AsyncLogger()
	{
		if (done_.load(std::memory_order_acquire))
		{
			return;
		}
		do_done();
	}

	void append_message(const std::string& msg);

	void wait_for_done()
	{
		if (done_.load(std::memory_order_acquire))
		{
			return;
		}
		do_done();
	}

  private:
	void run(const std::string& dir, const std::string& prefix,
			 std::size_t roll_size, std::chrono::seconds flush_interval,
			 std::size_t check_per_count, std::latch& start_latch);

	void do_done()
	{
		done_.store(true, std::memory_order_release);
		cv_.notify_one();

		if (thread_.joinable())
		{
			thread_.join();
		}
	}

  private:
	std::atomic<bool> done_{false};
	std::mutex mtx_;
	std::condition_variable cv_;

	std::jthread thread_;

	LoggerBuffer cur_buf_{};
	LoggerBuffer next_buf_{};

	std::vector<LoggerBuffer> bufs_;
};

inline void AsyncLogger::append_message(const std::string& msg)
{
	if (done_.load(std::memory_order_acquire))
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (!cur_buf_.full())
		{
			cur_buf_.push(msg);
			return;
		}

		bufs_.push_back(std::move(cur_buf_));
		if (next_buf_.check())
		{
			cur_buf_ = std::move(next_buf_);
		}
		else
		{
			cur_buf_ = LoggerBuffer();
		}

		cur_buf_.push(msg);
	}
	cv_.notify_one();
}

inline void AsyncLogger::run(const std::string& dir, const std::string& prefix,
							 std::size_t roll_size,
							 std::chrono::seconds flush_interval,
							 std::size_t check_per_count,
							 std::latch& start_latch)
{
	FileManager out_file(dir, prefix, roll_size, flush_interval,
						 check_per_count);

	LoggerBuffer new_buf1{};
	LoggerBuffer new_buf2{};

	std::vector<LoggerBuffer> bufs_to_write;
	bufs_to_write.reserve(16);

	while (!done_.load(std::memory_order_acquire))
	{
		{
			std::unique_lock<std::mutex> lock(mtx_);
			if (bufs_.empty())
			{
				bool initialized = [&start_latch]
				{
					start_latch.count_down();
					return true;
				}();

				cv_.wait_for(lock, std::chrono::seconds(flush_interval));
			}

			if (!cur_buf_.empty())
			{
				bufs_.push_back(std::move(cur_buf_));
				cur_buf_ = std::move(new_buf1);
			}

			bufs_to_write.swap(bufs_);

			if (!next_buf_.check())
			{
				next_buf_ = std::move(new_buf2);
			}
		}

		if (bufs_to_write.empty())
		{
			continue;
		}

		// 暂时禁用缓冲区数量限制，以便测试日志是否丢失
		// if (bufs_to_write.size() > 100)
		// {
		// 	std::string err = std::format(
		// 		"Dropped log messages at {}, {} larger LoggerBuffers\n",
		// 		std::chrono::system_clock::now(), bufs_to_write.size() - 2);
		// 	std::cerr << err;

		// 	out_file.append(err.c_str(), err.size());
		// 	bufs_to_write.erase(bufs_to_write.begin() + 2, bufs_to_write.end());
		// }

		for (const auto& buf : bufs_to_write)
		{
			for (const auto& msg : buf)
			{
				out_file.append(msg);
			}
		}

		if (bufs_to_write.size() < 2)
		{
			bufs_to_write.resize(2);
		}

		if (!new_buf1.check())
		{
			new_buf1 = std::move(bufs_to_write[0]);
			new_buf1.clear();
		}

		if (!new_buf2.check())
		{
			new_buf2 = std::move(bufs_to_write[1]);
			new_buf2.clear();
		}

		bufs_to_write.clear();
		out_file.flush();
	}

	out_file.flush();
}
} // namespace details
} // namespace elog

#endif
#ifndef ELOG_BUFFER_HPP
#define ELOG_BUFFER_HPP

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
namespace elog
{
namespace details
{
template <std::size_t N> struct Buffer
{
	using iterator = std::string*;
	using const_iterator = const std::string*;

	Buffer() : data_(std::make_unique<std::string[]>(N))
	{
	}

	Buffer(Buffer&& other)
		: data_(std::exchange(other.data_, nullptr)),
		  idx_(std::exchange(other.idx_, 0))
	{
	}

	Buffer& operator=(Buffer&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		data_ = std::exchange(other.data_, nullptr);
		idx_ = std::exchange(other.idx_, 0);
		return *this;
	}

	std::size_t size() const noexcept
	{
		return idx_;
	}

	bool check() const noexcept
	{
		return data_ != nullptr;
	}

	std::size_t capacity() const noexcept
	{
		return N;
	}

	bool empty() const noexcept
	{
		return idx_ == 0;
	}

	bool full() const noexcept
	{
		return idx_ == N;
	}

	void clear()
	{
		idx_ = 0;
	}

	void push(const std::string& msg)
	{
		assert(data_);
		data_[idx_++] = msg;
	}

	void push(std::string&& msg)
	{
		assert(data_);
		data_[idx_++] = std::move(msg);
	}

	iterator begin() noexcept
	{
		return data_.get();
	}
	iterator end() noexcept
	{
		return data_.get() + idx_;
	}

	const_iterator begin() const noexcept
	{
		return data_.get();
	}
	const_iterator end() const noexcept
	{
		return data_.get() + idx_;
	}

  private:
	std::unique_ptr<std::string[]> data_;
	std::size_t idx_{0};
};
} // namespace details
} // namespace elog

#endif
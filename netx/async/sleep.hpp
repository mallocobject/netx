#ifndef NETX_ASYNC_SLEEP_HPP
#define NETX_ASYNC_SLEEP_HPP

#include "netx/async/concepts.hpp"
#include "netx/async/event_loop.hpp"
#include "netx/async/task.hpp"
#include <chrono>
namespace netx
{
namespace async
{
template <typename Duration> struct SleepAwaiter
{
	explicit SleepAwaiter(Duration duration) : duration(duration)
	{
	}

	bool await_ready() const noexcept
	{
		return false;
	}

	template <Promise P>
	void await_suspend(std::coroutine_handle<P> coro) const noexcept
	{
		EventLoop::loop().call_after(duration, coro.promise());
	}

	void await_resume() const noexcept
	{
	}

  private:
	Duration duration;
};

template <typename Rep, typename Period>
[[nodiscard]] Task<> sleep(NoWaitAtInitialSuspend,
						   std::chrono::duration<Rep, Period> duration)
{
	co_await SleepAwaiter{duration};
}

template <typename Rep, typename Period>
[[nodiscard]] Task<> sleep(std::chrono::duration<Rep, Period> duration)
{
	return sleep(no_wait_at_initial_suspend, duration);
}

} // namespace async
} // namespace netx

#endif
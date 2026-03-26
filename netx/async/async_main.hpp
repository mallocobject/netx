#ifndef NETX_ASYNC_ASYNC_MAIN_HPP
#define NETX_ASYNC_ASYNC_MAIN_HPP

#include "netx/async/concepts.hpp"
#include "netx/async/event_loop.hpp"
#include "netx/async/scheduled_task.hpp"
#include <utility>
namespace netx
{
namespace async
{
template <Future Fut> decltype(auto) async_main(Fut&& main)
{
	auto task = ScheduledTask(std::move(main));
	EventLoop::loop().run_until_complete();
	return task.result();
}

inline void async_main()
{
	EventLoop::loop().run_until_complete();
}
} // namespace async
} // namespace netx

#endif
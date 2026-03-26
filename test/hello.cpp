#include "netx/async/awaitable_traits.hpp"
#include "netx/async/check_error.hpp"
#include "netx/async/concepts.hpp"
#include "netx/async/coro_handle.hpp"
#include "netx/async/epoll_poller.hpp"
#include "netx/async/event.hpp"
#include "netx/async/event_loop.hpp"
#include "netx/async/handle.hpp"
#include "netx/async/non_void_helper.hpp"
#include "netx/async/task.hpp"
#include <iostream>
#include <netx/async/async_main.hpp>
#include <string>

using namespace netx;

Task<std::string_view> hello()
{
	co_return "hello";
}

Task<std::string_view> world()
{
	co_return "world";
}

Task<std::string> hello_world()
{
	co_return std::format("{} {}", co_await hello(), co_await world());
}

int main()
{
	std::cout << std::format("run result: {}\n", async_main(hello_world()));

	return 0;
}
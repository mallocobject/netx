// #ifndef NETX_ASYNC_AND_THEN_HPP
// #define NETX_ASYNC_AND_THEN_HPP

// #include "netx/async/awaitable_traits.hpp"
// #include "netx/async/concepts.hpp"
// #include "netx/async/task.hpp"
// namespace netx
// {
// namespace async
// {
// template <Awaitable A, Awaitable F>
// 	requires(!std::invocable<F> &&
// 			 !std::invocable<F, typename AwaitableTraits<A>::RetType>)
// Task<typename AwaitableTraits<F>::RetType> and_then(A&& a, F&& f)
// {
// 	co_await std::forward<A>(a);
// 	co_return co_await std::forward<F>(f);
// }
// } // namespace async
// } // namespace netx

// #endif
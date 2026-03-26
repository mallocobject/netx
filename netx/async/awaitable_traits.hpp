#ifndef NETX_ASYNC_AWAITABLE_TRAITS_HPP
#define NETX_ASYNC_AWAITABLE_TRAITS_HPP

#include "netx/async/concepts.hpp"
#include "netx/async/non_void_helper.hpp"
namespace netx
{
namespace async
{
template <typename A> struct AwaitableTraits;

template <Awaiter A> struct AwaitableTraits<A>
{
	using RetType = decltype(std::declval<A>().await_resume());
	using NonVoidRetType = NonVoidHelper<RetType>::Type;
};

template <typename A>
	requires(!Awaiter<A> && Awaitable<A>)
struct AwaitableTraits<A>
	: AwaitableTraits<decltype(std::declval<A>().operator co_await())>
{
};
} // namespace async
} // namespace netx

#endif
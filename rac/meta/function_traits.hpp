#ifndef RAC_META_FUNCTION_TRAITS_HPP
#define RAC_META_FUNCTION_TRAITS_HPP

#include <tuple>
#include <type_traits>
namespace rac
{
template <typename T> struct FunctionTraits;

template <typename R, typename... Args> struct FunctionTraits<R (*)(Args...)>
{
	using RetType = R;
	using ArgsTuple = std::tuple<std::remove_cvref_t<Args>...>;
};

template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const>
{
	using RetType = R;
	using ArgsTuple = std::tuple<std::remove_cvref_t<Args>...>;
};

template <typename F>
struct FunctionTraits : FunctionTraits<decltype(&F::operator())>
{
};
} // namespace rac

#endif
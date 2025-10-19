#pragma once

namespace rsl::_log_impl {
template <typename...>
struct typelist {};
template <typename... Ts>
constexpr inline typelist<> make_empty{};

// TODO use this once clang-p2996 has been rebased onto upstream trunk
/*template <template <typename...> auto customization_point, typename... T>
constexpr inline auto&& customization = []<typename... U>(typelist<U...>) -> auto&& {
  return customization_point<U...>;
}(make_empty<T...>);*/

template <std::meta::info customization_point, typename... T>
constexpr inline auto&& customization = []<typename... U>(typelist<U...>) -> auto&& {
  return template[:customization_point:]<U...>;
}(make_empty<T...>);
}
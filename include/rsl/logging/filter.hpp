#pragma once
#include <tuple>
#include <type_traits>
#include <concepts>

#include "event.hpp"
#include "context.hpp"

namespace rsl::logging {
template <typename... Ts> struct Any;
template <typename... Ts> struct All;

struct Filter {
  template <typename T>  
  bool process_event(this T&& self,  Context const& ctx, Event const& event) {
    static_assert(requires { self.check(ctx, event.meta); }, "Filter does not implement `bool check(Metadata)`");
    return self.check(ctx, event.meta);
  }
  
  template <typename T>  
  bool process_context(this T&& self, Context const& ctx, Metadata const& meta, bool, bool) {
    static_assert(requires { self.check(ctx, meta); }, "Filter does not implement `bool check(Metadata)`");
    return self.check(ctx, meta);
  }

  template <typename T, typename L>
    requires (std::convertible_to<std::remove_cvref_t<L>, Filter>)
  auto operator>>(this T&& rhs, L&& lhs) {
    return All(std::forward<T>(rhs), std::forward<L>(lhs));
  }

  template <typename T, typename... Ts>
  auto operator>>(this T&& rhs, All<Ts...> lhs) {
    return All(All(std::forward<T>(rhs)), lhs);
  }

  template <typename T, typename U>
  auto operator>>(this T&& rhs, Any<U> lhs) {
    return std::forward<T>(rhs) >> get<0>(lhs.elts);
  }
};

template <typename... Ts>
struct All {
  std::tuple<Ts...> elts;

  template <typename... Us>
    requires ((std::same_as<std::remove_cvref_t<Us>, std::remove_cvref_t<Ts>> && ...))
  explicit All(Us&&... b) : elts(std::forward<Us>(b)...) {}

  template <typename... Vs, typename... Us>
  All(All<Vs...> rhs, All<Us...> lhs)
      : elts(std::tuple_cat(rhs.elts, lhs.elts)) {}

  template <typename... Vs, typename T>
  All(All<Vs...> rhs, T&& lhs)
      : elts(std::tuple_cat(rhs.elts, std::tuple<T>(std::forward<T>(lhs)))) {}

  bool process_event(Context const& ctx, Event const& event) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_event(ctx, event) && ...);
    }(std::index_sequence_for<Ts...>());
  }

  bool process_context(Context const& ctx, Metadata const& meta, bool entered, bool handover) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_context(ctx, meta, entered, handover) && ...);
    }(std::index_sequence_for<Ts...>());
  }

  template <typename T, typename L>
  requires (std::convertible_to<std::remove_cvref_t<L>, Filter>)
  auto operator>>(this T&& rhs, L&& lhs) {
    return rsl::logging::All(std::forward<T>(rhs), std::forward<L>(lhs));
  }

  template <typename T, typename L>
  auto operator>>(this T&& rhs, Any<L> lhs) {
    return std::forward<T>(rhs) >> get<0>(lhs.elts);
  }

  template <typename T, typename... Us>
  auto operator>>(this T&& rhs, All<Us...> lhs) {
    return rsl::logging::All(std::forward<T>(rhs), lhs);
  }
};

template <typename... Ts>
All(Ts&&...) -> All<Ts...>;

template <typename... Ts, typename... Us>
All(All<Ts...>, All<Us...>) -> All<Ts..., Us...>;

template <typename... Ts, typename T>
All(All<Ts...>, T&&) -> All<Ts..., T>;


template <typename... Ts>
struct Any : Filter {
  std::tuple<Ts...> elts;

  template <typename... Us>
    requires ((std::same_as<std::remove_cvref_t<Us>, std::remove_cvref_t<Ts>> && ...))
  explicit Any(Us&&... b) : elts(std::forward<Us>(b)...) {}

  bool process_event(Context const& ctx, Event const& event) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_event(ctx, event) || ...);
    }(std::index_sequence_for<Ts...>());
  }

  bool process_context(Context const& ctx, Metadata const& meta, bool entered, bool handover) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_context(ctx, meta, entered, handover) || ...);
    }(std::index_sequence_for<Ts...>());
  }
};

template <typename... Ts>
Any(Ts&&...) -> Any<Ts...>;

}
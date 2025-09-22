#pragma once
#include <tuple>
#include <type_traits>
#include <concepts>

#include "event.hpp"
#include "context.hpp"

namespace rsl::logging {
template <typename... Ts>
struct Any;
template <typename... Ts>
struct All;
struct Sink;

namespace _impl {
template <typename T>
constexpr inline bool will_terminate_impl = std::convertible_to<T, Sink>;

template <typename... Ts>
constexpr inline bool will_terminate_impl<All<Ts...>> =
    sizeof...(Ts) >= 1 && std::convertible_to<Ts...[sizeof...(Ts) - 1], Sink>;

template <typename... Ts>
constexpr inline bool will_terminate_impl<Any<Ts...>> = (will_terminate_impl<Ts> && ...);

template <typename T>
concept will_terminate = will_terminate_impl<T>;
}  // namespace _impl

struct Filter {
  template <typename T>
  bool process_event(this T&& self, Event const& event) {
    static_assert(
        requires { self.check(event.meta); },
        "Filter does not implement `bool check(Metadata)`");
    return self.check(event.meta);
  }

  template <typename T>
  bool process_context(this T&& self, Metadata const& meta, bool, bool) {
    static_assert(
        requires { self.check(meta); },
        "Filter does not implement `bool check(Metadata)`");
    return self.check(meta);
  }

  template <typename T, typename L>
    requires(not _impl::will_terminate<T> && std::convertible_to<std::remove_cvref_t<L>, Filter>)
  auto operator>>(this T&& rhs, L&& lhs) {
    return All(std::forward<T>(rhs), std::forward<L>(lhs));
  }

  template <typename T>
    requires _impl::will_terminate<T>
  auto operator>>(this T&& rhs, auto&&) = delete ("Unreachable");

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
    requires((std::same_as<std::remove_cvref_t<Us>, std::remove_cvref_t<Ts>> && ...))
  explicit All(Us&&... b) : elts(std::forward<Us>(b)...) {}

  template <typename... Vs, typename... Us>
  All(All<Vs...> rhs, All<Us...> lhs) : elts(std::tuple_cat(rhs.elts, lhs.elts)) {}

  template <typename... Vs, typename T>
  All(All<Vs...> rhs, T&& lhs)
      : elts(std::tuple_cat(rhs.elts, std::tuple<T>(std::forward<T>(lhs)))) {}

  bool process_event(Event const& event) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_event(event) && ...);
    }(std::index_sequence_for<Ts...>());
  }

  bool process_context(Metadata const& meta, bool entered, bool handover) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_context(meta, entered, handover) && ...);
    }(std::index_sequence_for<Ts...>());
  }

  template <typename T, typename L>
    requires(std::convertible_to<std::remove_cvref_t<L>, Filter>)
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
    requires((std::same_as<std::remove_cvref_t<Us>, std::remove_cvref_t<Ts>> && ...))
  explicit Any(Us&&... b) : elts(std::forward<Us>(b)...) {}

  bool process_event(Event const& event) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_event(event) || ...);
    }(std::index_sequence_for<Ts...>());
  }

  bool process_context(Metadata const& meta, bool entered, bool handover) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
      return (get<Idx>(elts).process_context(meta, entered, handover) || ...);
    }(std::index_sequence_for<Ts...>());
  }
};

template <typename... Ts>
Any(Ts&&...) -> Any<Ts...>;

template <typename F>
struct FilterFnc : Filter {
  F fnc;
  static_assert(requires(Metadata meta) { fnc(meta); });

  template <typename U>
    requires(std::same_as<std::remove_cvref_t<U>, F>)
  explicit FilterFnc(U&&) : fnc(std::forward<U>(fnc)) {}

  template <typename T>
  bool process_event(this T&& self, Event const& event) {
    return std::forward<T>(self).fnc(event.meta);
  }

  template <typename T>
  bool process_context(this T&& self, Metadata const& meta, bool, bool) {
    return std::forward<T>(self).fnc(meta);
  }
};
template <typename F>
FilterFnc(F&&) -> FilterFnc<F>;
}  // namespace rsl::logging
#pragma once
#include <cstddef>

#include <rsl/source_location>

#include "interface.hpp"
#include "context.hpp"

#ifndef RSL_LOG_CUSTOM
#  include "default_config.hpp"
#endif

namespace rsl::logging {
namespace _impl {
template <LogLevel Level, rsl::source_location sloc, std::meta::info ctx, rsl::string_view fmt, typename... Args>
Message make_message(Context const* context, Args&&... args) {
  // TODO expand format string
  return {.severity = Level,
          .sloc     = sloc,
          .span_id  = context ? context->id : 0,
          .text     = std::format(fmt, args...)};
}

template <LogLevel Level, typename... Args>
struct FormatString {
  using meta_t        = Message (*)(Context const*, Args&&...);
  meta_t make_message = nullptr;

  template <std::meta::info Ctx>
  consteval void maybe_initialize(std::string_view fmt, rsl::source_location sloc) {
    if constexpr (Level >= min_level_for(Ctx)) {
      make_message = extract<meta_t>(
          substitute(^^_impl::make_message,
            {
            std::meta::reflect_constant(Level),
            std::meta::reflect_constant(sloc),
            std::meta::reflect_constant(Ctx),
            std::meta::reflect_constant(rsl::string_view(std::define_static_string(fmt))),
                      ^^Args...}));
    }
  }
  using initialize_t = void (FormatString::*)(std::string_view, rsl::source_location);

  consteval explicit(false)
      FormatString(std::convertible_to<std::string_view> auto const& fmt,
                   rsl::source_location sloc = std::source_location::current(),
                   std::meta::info ctx       = std::meta::access_context::current().scope()) {
    // avoid instantiating make_message if this is ignored anyway
    if constexpr (Level >= global_min_level) {
      auto initializer =
          extract<initialize_t>(substitute(^^maybe_initialize, {reflect_constant(ctx)}));
      (this->*initializer)(std::string_view(fmt), sloc);
    }
  }
};
}  // namespace _impl

template <LogLevel S, typename... Args>
using FormatString = _impl::FormatString<S, std::type_identity_t<Args>...>;

template <typename T>
concept is_valid_logger = requires {
  { T::enter_span(std::declval<Context>(), true) } -> std::same_as<void>;
  { T::exit_span(std::declval<Context>(), true) } -> std::same_as<void>;
  { T::emit(nullptr, FormatString<LogLevel::INFO>("")) } -> std::same_as<void>;
};

static_assert(is_valid_logger<GlobalLogger>,
              "Global logger does not implement the logger protocol.");

template <LogLevel Level, typename... Args>
void emit_event(Context const* context, FormatString<Level, Args...> fmt, Args&&... args) {
  // we've already checked against global_min_level in FormatString's ctor
  // do it again here to avoid instantiating GlobalLogger::emit
  if constexpr (Level >= global_min_level) {
    // check context level override
    if (context != nullptr && Level < context->min_level) { return; }
    GlobalLogger::emit<Level>(context, fmt, std::forward<Args>(args)...);
  }
}


}  // namespace rsl::logging
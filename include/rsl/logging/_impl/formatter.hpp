#pragma once
//! do not include this header in any TUs of the logging library
//! this needs compile time options to be set - this is only the case in the consumer
#ifndef RSL_LOG_CUSTOM
#  include "default_config.hpp"
#endif

#include <cstddef>
#include <thread>
#include <chrono>

#include <rsl/source_location>

#include <rsl/logging/event.hpp>
#include <rsl/logging/context.hpp>
#include <rsl/logging/field.hpp>

namespace rsl::logging {
namespace _impl {
template <rsl::string_view fmt, typename... Args>
Event make_message(Metadata& meta, Args&&... args) {
  // TODO expand format string
  return {
      .meta = meta,
      .text = std::format(fmt, args...),
  };
}

template <LogLevel Level, typename... Args>
struct FormatString {
  using meta_t        = Event (*)(Metadata&, Args&&...);
  meta_t make_message = nullptr;

  rsl::source_location sloc;
  // TODO source_context
  template <std::meta::info Ctx>
  consteval void maybe_initialize(std::string_view fmt, rsl::source_location sloc) {
    if constexpr (Level >= min_level_for(Ctx)) {
      make_message = extract<meta_t>(
          substitute(^^_impl::make_message,
                     {std::meta::reflect_constant(rsl::string_view(std::define_static_string(fmt))),
                      ^^Args...}));
    }
  }
  using initialize_t = void (FormatString::*)(std::string_view, rsl::source_location);

  consteval explicit(false)
      FormatString(std::convertible_to<std::string_view> auto const& fmt,
                   rsl::source_location sloc = std::source_location::current(),
                   std::meta::info ctx       = std::meta::access_context::current().scope())
    : sloc(sloc) {
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

template <LogLevel Level, typename... Args>
void emit_event(ExtraFields const* fnc_args,
                Context const* context,
                FormatString<Level, Args...> fmt,
                Args&&... args) {
  // we've already checked against global_min_level in FormatString's ctor
  // do it again here to avoid instantiating GlobalLogger::emit
  if constexpr (Level >= global_min_level) {
    // check context level override
    if (context != nullptr && Level < context->min_level) {
      return;
    }
    auto meta = Metadata{.severity  = Level,
                         .timestamp = std::chrono::system_clock::now(),
                         .thread_id = std::this_thread::get_id(),
                         .context = context ? *context : Context(),
                         .arguments = fnc_args ? *fnc_args : ExtraFields{},
                         .sloc = fmt.sloc};

    RSL_LOG_EMITTER(meta, fmt, std::forward<Args>(args)...);
  }
}

}  // namespace rsl::logging
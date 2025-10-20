#pragma once
#include <rsl/logging/logger.hpp>
#include <rsl/logging/level.hpp>
#include <rsl/logging/event.hpp>

#include "util.hpp"

namespace rsl::logging {
#ifdef RSL_LOG_MIN_LEVEL
constexpr inline LogLevel global_min_level = [] {
  using enum LogLevel;  // enable omitting LogLevel::
  return parse_min_level(RSL_LOG_MIN_LEVEL);
}();
#else
#ifdef NDEBUG
constexpr inline LogLevel global_min_level = LogLevel::INFO;
#else
constexpr inline LogLevel global_min_level = LogLevel::DEBUG;
#endif
#endif

consteval bool is_enabled_for(LogLevel level) {
  return level >= global_min_level;
}

template <typename...>
constexpr inline auto selected_logger = DefaultLogger();

template <typename... Empty, typename... Sinks>
void set_output(Output<Sinks...>& sinks){
  static_assert(
      requires { selected_logger<Empty...>.set_output(sinks); },
      "Selected logger does not support dynamically configuring output");
  selected_logger<Empty...>.set_output(sinks);
}

template <typename... Empty>
void emit_context(Context const& ctx, bool entered, bool async_handover) {
  // ensure pack is empty
  rsl::_log_impl::customization<^^selected_logger, Empty...>.context(Metadata{.context=ctx}, entered, async_handover);
}

}  // namespace rsl::logging
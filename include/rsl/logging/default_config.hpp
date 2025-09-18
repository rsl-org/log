#pragma once
#include "logger.hpp"
#include "interface.hpp"

namespace rsl::logging {
#ifdef RSL_LOG_MIN_LEVEL
constexpr inline LogLevel global_min_level = [] {
  using enum LogLevel;  // enable omitting LogLevel::
  return parse_min_level(RSL_LOG_MIN_LEVEL);
}();
#else
constexpr inline LogLevel global_min_level = LogLevel::INFO;
#endif

#ifdef RSL_LOG_LAZY
#define RSL_LOG_EMITTER rsl::logging::lazy_emitter
#endif

#ifndef RSL_LOG_EMITTER
#define RSL_LOG_EMITTER rsl::logging::eager_emitter
#endif
}  // namespace rsl::logging
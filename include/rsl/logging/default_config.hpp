#pragma once
#include "logger.hpp"
#include "interface.hpp"

#ifdef RSL_LOG_ASYNC
#  ifdef RSL_LOG_IS_SYNCHRONOUS
#    error "Logger reincluded in asynchronous mode. Prior to this synchronous mode was used."
#  endif
#  ifndef RSL_LOG_IS_ASYNCHRONOUS
#    define RSL_LOG_IS_ASYNCHRONOUS
#  endif
#  include "mode/async.hpp"
#else
#  ifdef RSL_LOG_IS_ASYNCHRONOUS
#    error "Logger reincluded in synchronous mode. Prior to this asynchronous mode was used."
#  endif
#  ifndef RSL_LOG_IS_SYNCHRONOUS
#    define RSL_LOG_IS_SYNCHRONOUS
#  endif
#  include "mode/basic.hpp"
#endif

namespace rsl::logging {
#ifdef RSL_LOG_MIN_LEVEL
constexpr inline LogLevel global_min_level = [] {
  using enum LogLevel;  // enable omitting LogLevel::
  return parse_min_level(RSL_LOG_MIN_LEVEL);
}();
#else
constexpr inline LogLevel global_min_level = LogLevel::INFO;
#endif

#ifdef RSL_LOG_ASYNC
using GlobalLogger = AsyncLogger;
#else
using GlobalLogger = BasicLogger;

#endif
}  // namespace rsl::logging
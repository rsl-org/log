#pragma once
#include <rsl/logging/event.hpp>

namespace rsl::logging {
namespace _impl {
template <LogLevel Level, typename... Args>
struct FormatString;
}

struct NullLogger {
  static void context(Metadata const& meta, bool entered, bool async_handover) {}

  template <LogLevel Severity, typename... Args>
  static void emit(Metadata& meta, _impl::FormatString<Severity, Args...> fmt, Args&&... args) {}
};

}  // namespace rsl::logging
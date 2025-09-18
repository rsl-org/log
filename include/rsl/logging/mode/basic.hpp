#pragma once
#include <rsl/logging/interface.hpp>
#include <rsl/logging/context.hpp>

#include <print>

namespace rsl::logging {
namespace _impl {
template <LogLevel Level, typename... Args>
struct FormatString;
}

struct BasicLogger {
  static void enter_span(Context const& context, bool handover) {
    std::println("{}entered {}", handover ? "re" : "", context.name);
  }
  static void exit_span(Context const& context, bool handover) {
    std::println("{}exited {}", handover ? "" : "finally ", context.name);
  }
  template <LogLevel severity, typename... Args>
  static void emit(Context const* context, _impl::FormatString<severity, Args...> fmt, Args&&... args) {
    auto meta = fmt.make_message(context, std::forward<Args>(args)...);
    std::println("({: >8} {}) {}", context->name, context->id, meta.text);
    for (auto const& extra : context->arguments) {
      std::println("  {} = {}", extra.name, extra.to_string());
    }
    for (auto const& extra : context->extra) {
      std::println("  {} = {}", extra.name, extra.to_string());
    }
  }
};
}  // namespace rsl::logging
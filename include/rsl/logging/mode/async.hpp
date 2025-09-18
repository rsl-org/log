#pragma once
#include <rsl/logging/logger.hpp>
#include <rsl/logging/interface.hpp>
#include <rsl/logging/context.hpp>

#include <rsl/stream/memory.hpp>
#include <rsl/serialize>
#include "../_log_impl/buffer.hpp"

namespace rsl::logging {

namespace _impl {
template <LogLevel Level, typename... Args>
struct FormatString;
}

struct AsyncLogger {
  static void* message_queue();
  static void message_loop();

  static void enter_span(Context const& context, bool handover) {}
  static void exit_span(Context const& context, bool handover) {}
  template <LogLevel severity, typename... Args>
  static void emit(Context const* context, _impl::FormatString<severity, Args...> fmt, Args&&... args) {
    // std::vector<std::byte> buffer;
    // auto stream = stream::VectorOutputStream(buffer);
    // (to_bytes(stream, args), ...);
    // target().emit_message(severity, context, buffer);
  }
};
}  // namespace rsl::logging
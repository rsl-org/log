#include <rsl/logging/sinks.hpp>

namespace rsl::logging {
void TerminalSink::emit_event(Context const* ctx, Message const& event) {
  std::println("({: >8} {}) {}", ctx->name, ctx->id, event.text);
}

void TerminalSink::enter_context(Context const& ctx, bool handover) {
  std::println("entered {}", ctx.name);
  for (auto const& extra : ctx.arguments) {
    std::println("  {} = {}", extra.name, extra.to_string());
  }
  for (auto const& extra : ctx.extra) {
    std::println("  {} = {}", extra.name, extra.to_string());
  }
}

void TerminalSink::exit_context(Context const& ctx, bool handover) {
  std::println("exited {}", ctx.name);
}
}  // namespace rsl::logging
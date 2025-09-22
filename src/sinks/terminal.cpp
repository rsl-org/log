#include <chrono>
#include <print>
#include <rsl/logging/sinks.hpp>

namespace rsl::logging {
void TerminalSink::emit_event(Event const& event) {
  std::println("{} ({: >8} {}) {}", event.meta.timestamp, event.meta.context.name, event.meta.context.id, event.text);
}

void TerminalSink::enter_context(Metadata const& meta, bool handover) {
  std::println("entered {}", meta.context.name);
  for (auto const& extra : meta.context.arguments) {
    std::println("  {} = {}", extra.name, extra.to_string());
  }
  for (auto const& extra : meta.context.extra) {
    std::println("  {} = {}", extra.name, extra.to_string());
  }
}

void TerminalSink::exit_context(Metadata const& meta, bool handover) {
  std::println("exited {}", meta.context.name);
}
}  // namespace rsl::logging
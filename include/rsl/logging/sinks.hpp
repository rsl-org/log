#pragma once
#include "logger.hpp"

#include <print>

namespace rsl::logging {
struct TerminalSink final : Sink {
  void emit_event(Context const& ctx, Event const& event);
  void enter_context(Context const& ctx, Metadata const& meta, bool handover);
  void exit_context(Context const& ctx, Metadata const& meta, bool handover);
};
using DefaultSink = TerminalSink;

struct FileSink final : Sink {
  void emit_event(Context const& ctx, Event const& event);
  void enter_context(Context const& ctx, Metadata const& meta, bool handover);
  void exit_context(Context const& ctx, Metadata const& meta, bool handover);
};

// #if defined(__unix__) && defined(RSL_LOG_SYSTEMD)
struct SystemdSink final : Sink {
  void emit_event(Context const& ctx, Event const& event);
  void enter_context(Context const& ctx, Metadata const& meta, bool handover);
  void exit_context(Context const& ctx, Metadata const& meta, bool handover);
};
// #endif
}  // namespace rsl::loggging
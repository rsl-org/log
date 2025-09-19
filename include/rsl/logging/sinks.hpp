#pragma once
#include "logger.hpp"

#include <print>

namespace rsl::logging {
class TerminalSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override;
  void enter_context(Context const& ctx, bool handover) override;
  void exit_context(Context const& ctx, bool handover) override;
};
using DefaultSink = TerminalSink;

class FileSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override;
  void enter_context(Context const& ctx, bool handover) override;
  void exit_context(Context const& ctx, bool handover) override;
};

// #if defined(__unix__) && defined(RSL_LOG_SYSTEMD)
class SystemdSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override;
  void enter_context(Context const& ctx, bool handover) override;
  void exit_context(Context const& ctx, bool handover) override;
};
// #endif
}  // namespace rsl::loggging
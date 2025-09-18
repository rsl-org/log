#pragma once
#include "logger.hpp"

#include <print>

namespace rsl::logging {
class TerminalSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override {
    std::println("({: >8} {}) {}", ctx->name, ctx->id, event.text);
  }
  void enter_span(Context const& ctx, bool handover) override {
    std::println("entered {}", ctx.name);
    for (auto const& extra : ctx.arguments) {
      std::println("  {} = {}", extra.name, extra.to_string());
    }
    for (auto const& extra : ctx.extra) {
      std::println("  {} = {}", extra.name, extra.to_string());
    }
  }
  void exit_span(Context const& ctx, bool handover) override {
    std::println("exited {}", ctx.name);
  }
};
using DefaultSink = TerminalSink;

class FileSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override {}
  void enter_span(Context const& ctx, bool handover) override {}
  void exit_span(Context const& ctx, bool handover) override {}
};
class ThreadedFileSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override {}
  void enter_span(Context const& ctx, bool handover) override {}
  void exit_span(Context const& ctx, bool handover) override {}
};

#ifdef __unix__
class SystemdSink final : public Sink {
  void emit_event(Context const* ctx, Message const& event) override {}
  void enter_span(Context const& ctx, bool handover) override {}
  void exit_span(Context const& ctx, bool handover) override {}
};
#endif
}  // namespace rsl::loggging
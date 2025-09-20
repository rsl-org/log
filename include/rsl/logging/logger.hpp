#pragma once
#include "interface.hpp"
#include "context.hpp"
#include <print>

namespace rsl::logging {
namespace _impl {
template <LogLevel Level, typename... Args>
struct FormatString;
}

struct Sink {
  virtual ~Sink() = default;

  // TODO maybe query interest first to avoid formatting when no sinks are interested?
  // virtual bool is_interested(/* scope */) { return true; }

  virtual void emit_event(Context const* ctx, Message const& event) = 0;
  virtual void enter_context(Context const& ctx, bool handover) {}
  virtual void exit_context(Context const& ctx, bool handover) {}
};

struct BasicLogger : Sink {
  std::vector<Sink*> sinks;

  explicit BasicLogger(std::vector<Sink*> sinks) : sinks(std::move(sinks)) {}

  void emit_event(Context const* ctx, Message const& msg) override {
    for (auto sink : sinks) {
      sink->emit_event(ctx, msg);
    }
  }
  void enter_context(Context const& ctx, bool handover) override {
    for (auto sink : sinks) {
      sink->enter_context(ctx, handover);
    }
  }
  void exit_context(Context const& ctx, bool handover) override {
    for (auto sink : sinks) {
      sink->exit_context(ctx, handover);
    }
  }
};

Sink*& default_logger();
void set_default_logger(Sink* new_logger);

template <LogLevel severity, typename... Args>
void eager_emitter(ExtraFields const* fnc_args,
                   Context const* ctx,
                   _impl::FormatString<severity, Args...> fmt,
                   Args&&... args) {
  auto meta = fmt.make_message(fnc_args, ctx, std::forward<Args>(args)...);

  if (auto* logger = default_logger()) {
    logger->emit_event(ctx, meta);
  }
}
}  // namespace rsl::logging
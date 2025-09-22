#pragma once
#include "context.hpp"
#include "event.hpp"
#include "filter.hpp"


namespace rsl::logging {
namespace _impl {
template <LogLevel Level, typename... Args>
struct FormatString;
}

struct Sink : Filter {
  // This is the base class of all sinks. The expected member functions to customize 
  // this are not virtual.
  //
  // Expects:
  // void emit_event(Context const& ctx, Event const& event){}
  //
  // Optional: 
  // void enter_context(Context const& ctx, Metadata const& meta, bool handover) {}
  // void exit_context(Context const& ctx, Metadata const& meta, bool handover) {}

  template <typename T>  
  bool process_context(this T&& self, Context const& ctx, Metadata const& meta, bool entered, bool async_handover) {
    if (entered) {
      if constexpr (requires { self.enter_context(ctx, meta, async_handover); }) {
        self.enter_context(ctx, meta, async_handover);
      }
    } else {
      if constexpr (requires { self.exit_context(ctx, meta, async_handover); }) {
        self.exit_context(ctx, meta, async_handover);
      }
    }
    return false;
  }

  template <typename T>  
  bool process_event(this T&& self, Context const& ctx, Event const& event) {
    static_assert(requires { self.emit_event(ctx, event); }, "Sink does not implement `void emit_event(Context, Event)`");
    self.emit_event(ctx, event);
    return false;
  }
};

struct LoggerBase {
  virtual ~LoggerBase() = default;
  virtual void context(Context const& ctx, Metadata const& meta, bool entered, bool async_handover) = 0;
  virtual void emit(Context const& ctx, Event const& ev) = 0;
};

LoggerBase*& default_logger();
void set_default_logger(LoggerBase* new_logger);

template <typename... Ts>
struct Logger final : LoggerBase, Any<Ts...> {
  template <typename... Us>
    requires ((std::same_as<std::remove_cvref_t<Us>, std::remove_cvref_t<Ts>> && ...))
  explicit Logger(Us&&... values)
  : Any<Ts...>(std::forward<Us>(values)...) {}

  void context(Context const& ctx, Metadata const& meta, bool entered, bool async_handover) override {
    Any<Ts...>::process_context(ctx, meta, entered, async_handover);
  }

  void emit(Context const& ctx, Event const& ev) override {
    Any<Ts...>::process_event(ctx, ev);
  }

  void set_as_default() {
    set_default_logger(this);
  }

  ~Logger() override {
    // replace current logger with default logger if this is the current one
  }
};
template <typename... Ts>
Logger(Ts&&...) -> Logger<Ts...>;

template <LogLevel severity, typename... Args>
void eager_emitter(ExtraFields const* fnc_args,
                   Context const* ctx,
                   _impl::FormatString<severity, Args...> fmt,
                   Args&&... args) {
  auto meta = fmt.make_message(fnc_args, std::forward<Args>(args)...);

  if (auto* logger = default_logger()) {
    logger->emit(ctx ? *ctx : Context(), meta);
  }
}
}  // namespace rsl::logging
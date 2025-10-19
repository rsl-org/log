#pragma once
#include <thread>
#include <chrono>

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
  // void emit_event(Event const& event){}
  //
  // Optional:
  // void enter_context(Metadata const& meta, bool handover) {}
  // void exit_context(Metadata const& meta, bool handover) {}

  template <typename T>
  bool process_context(this T&& self, Metadata const& meta, bool entered, bool async_handover) {
    if (entered) {
      if constexpr (requires { self.enter_context(meta, async_handover); }) {
        self.enter_context(meta, async_handover);
      }
    } else {
      if constexpr (requires { self.exit_context(meta, async_handover); }) {
        self.exit_context(meta, async_handover);
      }
    }
    return false;
  }

  template <typename T>
  bool process_event(this T&& self, Event const& event) {
    static_assert(
        requires { self.emit_event(event); },
        "Sink does not implement `void emit_event(Event)`");
    self.emit_event(event);
    return false;
  }
};

struct LoggerBase {
  virtual ~LoggerBase()                                                         = default;
  virtual void context(Metadata const& meta, bool entered, bool async_handover) = 0;
  virtual void emit(Event const& ev)                                            = 0;
};

template <typename... Ts>
struct Output;

template <typename... Empty, typename... Sinks>
void set_output(Output<Sinks...>& sinks);

template <typename... Ts>
struct Output final
    : LoggerBase
    , Any<Ts...> {
  template <typename... Us>
    requires((std::same_as<std::remove_cvref_t<Us>, std::remove_cvref_t<Ts>> && ...))
  explicit Output(Us&&... values) : Any<Ts...>(std::forward<Us>(values)...) {}

  void context(Metadata const& meta, bool entered, bool async_handover) override {
    Any<Ts...>::process_context(meta, entered, async_handover);
  }

  void emit(Event const& ev) override { Any<Ts...>::process_event(ev); }

  void set_as_default() && = delete;
  void set_as_default() & { set_output(*this); }
};
template <typename... Ts>
Output(Ts&&...) -> Output<Ts...>;

struct NullLogger {
  static void context(Metadata const& meta, bool entered, bool async_handover) {}

  template <LogLevel Severity, typename... Args>
  static void emit(Metadata& meta, _impl::FormatString<Severity, Args...> fmt, Args&&... args) {}
};

struct DefaultLogger {
  static void context(Metadata const& meta, bool entered, bool async_handover) {
    if (auto* logger = output()) {
      logger->context(meta, entered, async_handover);
    }
  }

  template <LogLevel Severity, typename... Args>
  static void emit(Metadata& meta, _impl::FormatString<Severity, Args...> fmt, Args&&... args) {
    auto message = fmt.make_message(std::forward<Args>(args)...);
    auto event   = Event{.meta = meta, .text = message};
    if (auto* logger = output()) {
      logger->emit(event);
    }
  }

  static LoggerBase*& output();
  template <typename... Sinks>
  static void set_output(Output<Sinks...>& logger) {
    output() = &logger;
  }
};

}  // namespace rsl::logging
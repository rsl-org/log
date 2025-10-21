#pragma once
#include <rsl/logging/event.hpp>
#include <rsl/logging/output.hpp>

namespace rsl::logging {
namespace _impl {
template <LogLevel Level, typename... Args>
struct FormatString;
}

struct DefaultLogger {
  static void context(Metadata const& meta, bool entered, bool async_handover) {
    if (auto* output = current_output()) {
      output->context(meta, entered, async_handover);
    }
  }

  template <LogLevel Severity, typename... Args>
  static void emit(Metadata& meta, _impl::FormatString<Severity, Args...> fmt, Args&&... args) {
    auto message = fmt.make_message(std::forward<Args>(args)...);
    auto event   = Event{.meta = meta, .text = message};
    if (auto* output = current_output()) {
      output->emit(event);
    }
  }

  static void set_output(OutputBase& output) {
    current_output() = &output;
  }

private:
  static OutputBase*& current_output();
};

}  // namespace rsl::logging
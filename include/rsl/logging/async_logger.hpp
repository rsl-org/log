#include "logger.hpp"

namespace rsl::logging {
struct AsyncLogger : DefaultLogger {
  // message queue

  void context(Metadata const& meta, bool entered, bool async_handover) {
    // serialize
    // submit to message queue
  }

  template <LogLevel Severity, typename... Args>
  void emit(Metadata& meta, _impl::FormatString<Severity, Args...> fmt, Args&&... args) {
    auto formatter = fmt.make_message;
    // serialize metadata
    // serialize formatter
    // serialize arguments
    // submit to message queue
  }

  // entrypoint for logging thread
  static void process_messages() {
    // pop from queue
    // TODO handle context changes

    // deserialize formatter + arguments
    // format
    //   auto event = Event{/*TODO*/};

    //   if (auto* logger = current_output()) {
    //     logger->emit(event);
    //   }
  }
};
}  // namespace rsl::logging
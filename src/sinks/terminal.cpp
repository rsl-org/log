
#include <rsl/logging/logger.hpp>

namespace rsl::logging {
struct TerminalSink final : Sink {
  ~TerminalSink() override = default;
  void on_message(LogLevel level,
                  std::string_view message,
                  rsl::source_location location) override {

  };
};
}  // namespace rsl::logging
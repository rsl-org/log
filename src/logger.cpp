#include <rsl/logging/logger.hpp>
#include <rsl/logging/sinks.hpp>

namespace rsl::logging {
LoggerBase* get_default_logger() {
  static auto logger = Logger(TerminalSink());
  return &logger;
}

LoggerBase*& default_logger() {
  static LoggerBase* current = get_default_logger();
  return current;
}

void set_default_logger(LoggerBase* new_logger) {
  default_logger() = new_logger;
}
}  // namespace rsl::logging
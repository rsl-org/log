#include <rsl/logging/logger.hpp>
#include <rsl/logging/sinks.hpp>

namespace rsl::logging {
namespace {
LoggerBase* get_default_logger() {
  static auto logger = Output(TerminalSink());
  return &logger;
}
}  // namespace

LoggerBase*& DefaultLogger::current_output() {
  static LoggerBase* current = get_default_logger();
  return current;
}

}  // namespace rsl::logging
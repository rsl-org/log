#include <rsl/logging/logger.hpp>
#include <rsl/logging/sinks.hpp>

namespace rsl::logging {
namespace {
OutputBase* get_default_logger() {
  static auto logger = Output(TerminalSink());
  return &logger;
}
}  // namespace

OutputBase*& DefaultLogger::current_output() {
  static OutputBase* current = get_default_logger();
  return current;
}

}  // namespace rsl::logging
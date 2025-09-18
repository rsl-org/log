#include <rsl/logging/logger.hpp>
#include <rsl/logging/sinks.hpp>
namespace rsl::logging {
Sink*& default_logger() {
  static DefaultSink default_sink{};
  static BasicLogger default_logger{{&default_sink}};
  static Sink* current = &default_logger;
  return current;
}

void set_default_logger(Sink* new_logger) {
  default_logger() = new_logger;
}
}  // namespace rsl::logging
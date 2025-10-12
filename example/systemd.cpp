#define RSL_DOLLAR_MACROS

#include <rsl/log>
#include <rsl/logging/sinks.hpp>

void bar(int x) {
  rsl::info("from bar");

  $info("from bar with context");

}

void foo(int x, char y) {
  int test = 42;
  $context("boings", rsl::log_level::INHERIT);
  $error("from foo: {}", 24);
  bar(420);
}


int main() {
  using namespace rsl::logging;
  auto error_sink = TerminalSink();
  auto logger = Logger(
    // SystemdSink(), 
    TerminalSink(),
    filter(event->severity >= rsl::log_level::ERROR) >> error_sink,
    filter([](auto const& event){ return event.severity >= rsl::log_level::ERROR; }) >> error_sink
  );
  logger.set_as_default();
  {
    $context("main", rsl::log_level::INFO);
    rsl::info("from main context");
    foo(42, 'c');
  }
  rsl::warn("global context");
}
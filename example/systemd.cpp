#define RSL_DOLLAR_MACROS

#include <rsl/log>
#include <rsl/logging/sinks.hpp>

void bar(int x) {
  rsl::info("from bar");

  $info("from bar with context");

}

void foo(int x, char y) {
  int test = 42;
  $context("boings", rsl::log_level::INHERIT, zoinks = 420, boings = 3, test);
  $error("from foo: {}", 24);
  bar(420);
}

int main() {
  using namespace rsl::logging;
  auto logger = Logger(SystemdSink(), TerminalSink());
  logger.set_as_default();
  {
    $context("main", rsl::log_level::INFO);
    rsl::info("from main context");
    foo(42, 'c');
  }
  rsl::warn("global context");
}
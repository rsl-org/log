#define RSL_DOLLAR_MACROS

#include <rsl/log>
#include <rsl/logging/sinks.hpp>

void bar() {
  rsl::info("from bar");
}

void foo(int x, char y) {
  int test = 42;
  $context("boings", rsl::log_level::INHERIT, zoinks = 420, boings = 3, test);

  rsl::fatal_error("from foo");
  bar();
}

int main() {
  auto sink   = rsl::logging::SystemdSink();
  auto logger = rsl::logging::BasicLogger({&sink});
  rsl::logging::set_default_logger(&logger);
  {
    $context("main", rsl::log_level::INFO);
    rsl::info("from main context");
    foo(42, 'c');
  }
  rsl::warn("global context");
}
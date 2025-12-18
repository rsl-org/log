#define RSL_DOLLAR_MACROS
#include <rsl/log>

void bar() {
  $context("bar", rsl::log_level::INFO, x=1, y=2);
  rsl::info("from bar");
}

void foo() {
  rsl::info("before bar");
  bar();
  rsl::info("after bar");
}

int main() {
  rsl::error("error before main context");

  auto x = 42;
  auto ctx = rsl::log::context("main", rsl::log_level::INFO);
  ctx.enter();
  rsl::error("test error", $args(foo=123));
  foo();
  ctx.exit();
}
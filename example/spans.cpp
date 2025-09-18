#include <rsl/log>

using enum rsl::logging::LogLevel;

void bar() {
  rsl::logging::ContextGuard context{"main", INFO};
  rsl::info("from bar");
}

void foo() {
  rsl::info("before bar");
  bar();
  rsl::info("after bar");
}

int main() {
  rsl::error("error before main context");

  // rsl::logging::ContextGuard context{INFO, "main"};
  auto x = 42;
  auto ctx = rsl::log::context("main", INFO);
  ctx.extra = rsl::_log_impl::ExtraFields{{rsl::_log_impl::Field(^^x).set_ptr(&x)}};
  ctx.enter();
  rsl::error("test error");
  foo();
  ctx.exit();
}
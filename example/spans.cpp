#include <rsl/log>

void bar() {
  // TODO
  RSL_LOG_CONTEXT("bar", rsl::log_level::INFO, x=1, y=2);
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
  auto ctx = rsl::log::context("main", rsl::log_level::INFO);
  // ctx.extra = ExtraFields{{rsl::_log_impl::Field(^^x).set_ptr(&x)}};
  ctx.enter();
  rsl::error("test error");
  foo();
  ctx.exit();
}
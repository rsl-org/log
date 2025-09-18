#include <rsl/log>
#include <coroutine>
#include <thread>

struct Task {
  struct promise_type {
    Task get_return_object() noexcept {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() { std::terminate(); }
  };

  std::coroutine_handle<promise_type> h;
  explicit Task(std::coroutine_handle<promise_type> h) : h(h) {}
  ~Task() {
    if (h)
      h.destroy();
  }
};

rsl::co_trace<Task> example() {
  auto _ = rsl::logging::ContextGuard(rsl::logging::LogLevel::INHERIT, "example");
  rsl::info("Coroutine started on thread {}", std::this_thread::get_id());
  co_await std::suspend_always{};  // suspend once
  rsl::info("Coroutine resumed on thread {}", std::this_thread::get_id());
}

int main() {
  rsl::info("Before main context");
  auto _ = rsl::logging::ContextGuard(rsl::logging::LogLevel::DEBUG, "main");
  rsl::info("After main context");
  auto ex = example();

  std::thread worker([handle = ex.handle()]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    rsl::info("Resuming coroutine from worker thread {}", std::this_thread::get_id());
    handle.resume();
  });

  worker.join();
  rsl::info("final");
}

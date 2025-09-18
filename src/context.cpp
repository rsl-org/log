#include <thread>
#include <cassert>
#include <print>
#include <atomic>

#include <rsl/logging/context.hpp>

namespace rsl::logging {

void Context::enter(bool handover) {
  std::println("a {} -> {:x} <-> {:x} ({} {} <-> {} {})", 
    std::this_thread::get_id(),
    std::uintptr_t(current_context),
    std::uintptr_t(this),
    current_context->name,
    current_context->id,
    name,
    id);
  assert(parent == nullptr);
  parent       = current_context;
  current_context = this;
}

void Context::exit(bool handover) {
  // TODO figure out why this doesn't always hold
  // assert(current_context == this); 
  std::println("d {} -> {:x} <-> {:x} ({} {} <-> {} {})", 
    std::this_thread::get_id(),
    std::uintptr_t(current_context),
    std::uintptr_t(this),
    current_context->name,
    current_context->id,
    name,
    id);
  current_context = parent;
  parent       = nullptr;
}

bool Context::enabled_for(LogLevel level) const {
  if (min_level == LogLevel::INHERIT) {
    return parent != nullptr && parent->enabled_for(level);
  } else {
    return level >= min_level;
  }
}

Context* Context::get_default() {
  // TODO decide how to mutate default context in a thread-safe manner
  static Context default_span{"global", LogLevel::INFO};
  return &default_span;
}

std::size_t Context::next_id() {
  static std::atomic_unsigned_lock_free counter{1};
  return counter.fetch_add(1);
}
}  // namespace rsl::logging
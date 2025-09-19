#include <thread>
#include <cassert>
#include <print>
#include <atomic>

#include <rsl/logging/context.hpp>
#include <rsl/logging/logger.hpp>

namespace rsl::logging {
thread_local Context* current_context = Context::get_default();

void Context::enter(bool handover) {
  assert(parent == nullptr);
  parent          = current_context;
  current_context = this;
  default_logger()->enter_context(*this, handover);
}

void Context::exit(bool handover) {
  default_logger()->exit_context(*this, handover);
  if (current_context && current_context->id == id) {
    // fast path - we were are in the current context
    current_context = current_context->parent;
    parent          = nullptr;
    return;
  }

  for (Context* current = current_context; current != nullptr && current->parent; current = current->parent) {
    if (current->parent->id == id) {
      current->parent = current->parent->parent;
      parent          = nullptr;
      return;
    }
  }

  // TODO throw?
  std::unreachable();
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
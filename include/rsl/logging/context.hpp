#pragma once
#include <string>
#include <coroutine>

#include <rsl/source_location>
#include <rsl/_impl/consumable.hpp>
#include "level.hpp"
#include "field.hpp"

namespace rsl::logging {
struct Context;
template <typename... Empty>
void emit_context(Context const& meta, bool entered, bool async_handover);

struct RSL_CONSUMABLE(unconsumed) Context {
  Context* parent    = nullptr;
  LogLevel min_level = LogLevel::INHERIT;
  std::size_t id     = 0;
  std::string name;
  ExtraFields arguments;
  ExtraFields extra;
  rsl::source_location sloc;

  Context() = default;

  RSL_RETURN_TYPESTATE(unconsumed)
  Context(std::string name,
          LogLevel min_level,
          ExtraFields arguments            = {},
          ExtraFields extra                = {},
          rsl::source_location const& sloc = std::source_location::current())
      : min_level(min_level)
      , id(next_id())
      , name(std::move(name))
      , arguments(std::move(arguments))
      , extra(std::move(extra))
      , sloc(sloc) {}
  // shallow copy
  RSL_RETURN_TYPESTATE(unconsumed) Context(Context const& other) = default;

  [[nodiscard]] Context clone() const {
    Context cloned{};
    cloned.parent    = nullptr;
    cloned.min_level = min_level;
    cloned.id        = id;
    cloned.name      = name;
    cloned.arguments = arguments.clone();
    cloned.extra     = extra.clone();
    return cloned;
  }

  static std::size_t next_id();
  static Context* get_default();

  template <typename... E>
  RSL_CALLABLE_WHEN(unconsumed)
  RSL_SET_TYPESTATE(consumed) void enter(bool handover = false) {
    activate();
    emit_context<E...>(*this, true, handover);
  }

  template <typename... E>
  RSL_CALLABLE_WHEN(consumed)
  RSL_SET_TYPESTATE(unconsumed) void exit(bool handover = false) {
    if (deactivate()) {
      emit_context<E...>(*this, false, handover);
    }
  }

  [[nodiscard]] bool enabled_for(LogLevel level) const;

private:
  RSL_CALLABLE_WHEN(unconsumed)
  RSL_SET_TYPESTATE(consumed)
  void activate();

  RSL_CALLABLE_WHEN(consumed)
  RSL_SET_TYPESTATE(unconsumed)
  bool deactivate();
};

extern thread_local Context* current_context;

template <typename T = std::monostate>
struct ContextGuard : private Context {
  T extra;

  explicit ContextGuard(std::string name,
                        LogLevel min_level,
                        T extra = {},
                        ExtraFields arguments            = {},
                        rsl::source_location const& sloc = std::source_location::current())
      : Context(name, min_level, arguments, {}, sloc) {
    // TODO bind T as ExtraFields
    enter<T>();
  }

  ContextGuard(ContextGuard const&)            = delete;
  ContextGuard(ContextGuard&&)                 = delete;
  ContextGuard& operator=(ContextGuard const&) = delete;
  ContextGuard& operator=(ContextGuard&&)      = delete;

  ~ContextGuard() { exit<T>(); }

  using Context::enabled_for;
};


namespace _log_impl {
template <typename T>
concept has_member_op_co_await = requires(T t) { t.operator co_await(); };

template <typename T>
concept has_free_op_co_await = requires(T t) { operator co_await(t); };

template <typename T>
decltype(auto) to_awaiter(T&& t) {
  if constexpr (has_member_op_co_await<T>) {
    return std::forward<T>(t).operator co_await();
  } else if constexpr (has_free_op_co_await<T>) {
    return operator co_await(std::forward<T>(t));  // ADL
  } else {
    return std::forward<T>(t);  // already an awaiter
  }
}

template <typename Awaitable, typename Promise>
  requires(requires(Promise* p) { p->saved_span; })
struct AwaiterWrapper {
  Awaitable original;
  Promise* promise;

  bool await_ready() noexcept(noexcept(to_awaiter(original).await_ready())) {
    return to_awaiter(original).await_ready();
  }

  template <typename H>
  decltype(auto) await_suspend(H h) noexcept(noexcept(to_awaiter(original).await_suspend(h))) {
    promise->saved_span    = Context{current_context->name, current_context->min_level};
    promise->saved_span.id = current_context->id;
    current_context->exit<Awaitable>(true);
    return to_awaiter(original).await_suspend(h);
  }

  decltype(auto) await_resume() noexcept(noexcept(to_awaiter(original).await_resume())) {
    promise->saved_span.template enter<Awaitable>(true);
    return to_awaiter(original).await_resume();
  }
};
}  // namespace _log_impl

template <typename R>
struct Trace {
  constexpr static bool _rsl_logging_enabled = true;

  struct promise_type : R::promise_type {
    using Base = typename R::promise_type;
    Context saved_span{};

    Trace get_return_object() noexcept(noexcept(
        std::coroutine_handle<promise_type>::from_promise(std::declval<promise_type&>()))) {
      return Trace{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    template <typename Awaitable>
    auto await_transform(Awaitable&& aw) {
      auto transformed = [&]() {
        if constexpr (requires(Base b, Awaitable a) {
                        b.await_transform(std::forward<Awaitable>(a));
                      }) {
          return static_cast<Base&>(*this).await_transform(std::forward<Awaitable>(aw));
        } else {
          return std::forward<Awaitable>(aw);
        }
      }();

      return _log_impl::AwaiterWrapper<decltype(transformed), promise_type>{std::move(transformed),
                                                                            this};
    }
  };

  std::coroutine_handle<promise_type> co_handle;

  explicit Trace(std::coroutine_handle<promise_type> hh) : co_handle(hh) {}
  Trace(Trace&& other) noexcept : co_handle(swap(other.co_handle, {})) {}
  Trace& operator=(Trace&& other) noexcept {
    if (this != &other) {
      if (co_handle) {
        co_handle.destroy();
      }
      co_handle = swap(other.co_handle, {});
    }
    return *this;
  }

  ~Trace() {
    if (co_handle) {
      co_handle.destroy();
    }
  }

  std::coroutine_handle<promise_type> handle() const noexcept { return co_handle; }
};
}  // namespace rsl::logging
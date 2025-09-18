#pragma once
#include <atomic>
#include <cstddef>
#include <new>
#include <thread>
#include <stop_token>

namespace rsl::_log_impl {
struct QueueBase {
  template <typename T>
  auto pop(this T&& self, std::stop_token const& token) -> std::remove_cvref_t<T>::element_type {
    typename std::remove_cvref_t<T>::element_type obj;

    while (!std::forward<T>(self).try_pop(&obj)) {
      if (token.stop_requested()) {
        return {};
      }
      std::this_thread::yield();
    }
    return obj;
  }

  template <typename T>
  auto pop(this T&& self) -> std::remove_cvref_t<T>::element_type {
    typename std::remove_cvref_t<T>::element_type obj;
    while (!std::forward<T>(self).try_pop(&obj)) {
      std::this_thread::yield();
    }
    return obj;
  }

  template <typename T, typename U>
  void push(this T&& self, U&& obj) {
    while (!std::forward<T>(self).try_push(std::forward<U>(obj))) {
      std::this_thread::yield();
    }
  }
};
template <typename T, std::size_t N>
struct BoundedMPMC : QueueBase {
  static_assert(N >= 2, "Must be able to store at least 2 elements.");
  static_assert((N & (N - 1)) == 0, "Maximum number of elements must be power of 2.");
  static constexpr auto buffer_mask    = N - 1;
  constexpr static auto cacheline_size = std::hardware_destructive_interference_size;
  using element_type                   = T;
  static constexpr auto capacity       = N;

  BoundedMPMC() {
    for (std::size_t i = 0; i != N; i += 1) {
      buffer[i].sequence.store(i, std::memory_order_relaxed);
    }
    write_pos.store(0, std::memory_order_relaxed);
    read_pos.store(0, std::memory_order_relaxed);
  }
  BoundedMPMC(BoundedMPMC const&)    = delete;
  void operator=(BoundedMPMC const&) = delete;

  template <typename U>
  bool try_push(U&& data) {
    Cell* cell = nullptr;

    auto pos = write_pos.load(std::memory_order_relaxed);
    while (true) {
      cell     = &buffer[pos & buffer_mask];
      auto seq = cell->sequence.load(std::memory_order_acquire);
      auto dif = (intptr_t)seq - (intptr_t)pos;
      if (dif == 0) {
        if (write_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        return false;
      } else {
        pos = write_pos.load(std::memory_order_relaxed);
      }
    }

    new (&cell->data) T(std::forward<U>(data));
    cell->sequence.store(pos + 1, std::memory_order_release);

    return true;
  }

  bool try_pop(T* target) {
    Cell* cell = nullptr;

    auto pos = read_pos.load(std::memory_order_relaxed);
    while (true) {
      cell     = &buffer[pos & buffer_mask];
      auto seq = cell->sequence.load(std::memory_order_acquire);
      auto dif = (intptr_t)seq - (intptr_t)(pos + 1);
      if (dif == 0) {
        if (read_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        return false;
      } else {
        pos = read_pos.load(std::memory_order_relaxed);
      }
    }

    new (target) T(std::move(cell->data));
    cell->sequence.store(pos + buffer_mask + 1, std::memory_order_release);
    return true;
  }

  bool is_empty() const {
    auto pos   = read_pos.load(std::memory_order_relaxed);
    auto* cell = &buffer[pos & buffer_mask];
    auto seq   = cell->sequence.load(std::memory_order_acquire);
    auto dif   = (intptr_t)seq - (intptr_t)(pos + 1);
    return dif < 0;
  }

private:
  struct Cell {
    std::atomic<std::size_t> sequence;
    T data;
  };

  Cell buffer[N];
  alignas(cacheline_size) std::atomic<std::size_t> write_pos;
  alignas(cacheline_size) std::atomic<std::size_t> read_pos;
};
}  // namespace rsl::_log_impl
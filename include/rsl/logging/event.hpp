#pragma once
#include <thread>
#include <chrono>

#include <rsl/format>
#include <rsl/meta_traits>
#include <rsl/source_location>

#include "level.hpp"
#include "context.hpp"
#include "field.hpp"

namespace rsl::logging {

struct Metadata {
  LogLevel severity;
  std::chrono::system_clock::time_point timestamp;
  std::thread::id thread_id;
  Context context;
  ExtraFields arguments;

  // set by the formatter
  rsl::source_location sloc;
};

struct Event {
  Metadata meta;
  format_result text;

  //[[=getter]]
  std::uint64_t unix_timestamp() const {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(meta.timestamp.time_since_epoch())
            .count());
  }
};
}  // namespace rsl::logging
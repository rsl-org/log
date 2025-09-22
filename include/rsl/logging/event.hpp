#pragma once
#include <rsl/meta_traits>
#include <rsl/source_location>

#include "level.hpp"
#include "context.hpp"
#include "field.hpp"

namespace rsl::logging {

struct Metadata {
  LogLevel severity;
  // timestamp
  // thread id
  rsl::source_location sloc;
  ExtraFields arguments;
};

struct Event {
  Metadata meta;
  std::string text;
};
}  // namespace rsl::logging
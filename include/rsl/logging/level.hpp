#pragma once
#include <cstdint>
#include <type_traits>
#include <concepts>
#include <string_view>

#include <rsl/meta_traits>
#include <rsl/source_location>

namespace rsl::logging {
enum class LogLevel : std::uint8_t {
  INHERIT = 0,
  CONTEXT = 1,
  TRACE   = 10,
  DEBUG   = 20,
  INFO    = 30,
  WARNING = 40,
  ERROR   = 50,
  FATAL   = 60,
  DISABLE = 255,
};

template <typename T>
consteval LogLevel parse_min_level(T value) {
  if constexpr (std::same_as<T, LogLevel>) {
    return value;
  } else if constexpr (std::is_integral_v<T>) {
    return LogLevel(value);
  } else if (std::convertible_to<T const&, std::string_view>) {
    auto name = std::string_view(value);
    for (auto enumerator : enumerators_of(^^LogLevel)) {
      if (name == identifier_of(enumerator)) {
        return extract<LogLevel>(constant_of(enumerator));
      }
    }
    throw "invalid level name";
  }
  throw "unrecognized min level type";
}

consteval LogLevel min_level_for(std::meta::info ctx) {
  while (ctx != ^^::) {
    if (meta::has_annotation(ctx, ^^LogLevel)) {
      return extract<LogLevel>(constant_of(meta::get_annotation(ctx, ^^LogLevel)));
    }
    ctx = parent_of(ctx);
  }
  return LogLevel::INHERIT;
}

}  // namespace rsl::logging
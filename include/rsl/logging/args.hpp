#pragma once
#include <meta>
#include <ranges>

#include "field.hpp"

namespace rsl::logging {
namespace _impl {

constexpr inline struct Tombstone {
} tombstone;
}  // namespace _impl

template <std::meta::info scope = std::meta::access_context::current().scope()>
  requires(is_function(scope))
struct FunctionScope {
  static constexpr auto members = define_static_array(parameters_of(scope));
  static constexpr auto max_idx = members.size();

  static consteval std::meta::info get(std::size_t idx) {
    if (idx >= max_idx) {
      return std::meta::reflect_constant(_impl::tombstone);
    } else {
      return variable_of(parameters_of(scope)[idx]);
    }
  }

  template <typename... Ts>
  static ExtraFields capture_args(Ts&&... args) {
    std::array<Field, max_idx> arguments;
    template for (constexpr auto Idx : std::views::iota(0ZU, max_idx)) {
      constexpr static auto param = parameters_of(scope)[Idx];
      constexpr static auto name  = define_static_string(identifier_of(param));

      typename[:type_of(param):]* ptr = nullptr;
      if constexpr (sizeof...(Ts) >= Idx) {
        ptr = args...[Idx];
      }
      arguments[Idx] = Field(name, ptr);
    }

    return ExtraFields({arguments.begin(), arguments.end()});
  }
};
}  // namespace rsl::logging

#define RSL_DUMP_ARGS1(Offset)                             \
  (Offset < rsl::logging::FunctionScope<>::max_idx       \
   ? &[:rsl::logging::FunctionScope<>::get(Offset + 0):] \
   : nullptr)

#define RSL_DUMP_ARGS4(Offset)                                                    \
  RSL_DUMP_ARGS1(Offset), RSL_DUMP_ARGS1(Offset + 1), RSL_DUMP_ARGS1(Offset + 2), \
      RSL_DUMP_ARGS1(Offset + 3)
#define RSL_DUMP_ARGS16(Offset)                                                       \
  RSL_DUMP_ARGS4(Offset + 0), RSL_DUMP_ARGS4(Offset + 4), RSL_DUMP_ARGS4(Offset + 8), \
      RSL_DUMP_ARGS4(Offset + 12)

#ifndef RSL_ARGDUMP_COUNT
#  define RSL_ARGDUMP_COUNT 32
#endif
#if RSL_ARGDUMP_COUNT == 64
#  define RSL_LOG_ARGS                                                 \
    rsl::logging::FunctionScope<>::capture_args(RSL_DUMP_ARGS16(0),  \
                                                  RSL_DUMP_ARGS16(16), \
                                                  RSL_DUMP_ARGS16(32), \
                                                  RSL_DUMP_ARGS16(48))
#elif RSL_ARGDUMP_COUNT == 32
#  define RSL_LOG_ARGS \
    rsl::logging::FunctionScope<>::capture_args(RSL_DUMP_ARGS16(0), RSL_DUMP_ARGS16(16))
#elif RSL_ARGDUMP_COUNT == 16
#  define RSL_LOG_ARGS rsl::logging::FunctionScope<>::capture_args(RSL_DUMP_ARGS16(0))
#elif RSL_ARGDUMP_COUNT == 8
#  define RSL_LOG_ARGS \
    rsl::logging::FunctionScope<>::capture_args(RSL_DUMP_ARGS4(0), RSL_DUMP_ARGS4(4))
#else
#  error "RSL_ARGDUMP_COUNT must be power of 2"
#endif
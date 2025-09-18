#pragma once
#include <meta>
#include <algorithm>
#include <initializer_list>

namespace rsl::_log_impl {
constexpr inline struct Tombstone{} tombstone;

consteval std::size_t first_tombstone_idx(std::initializer_list<std::meta::info> ts) {
  auto it = std::ranges::find(ts, ^^Tombstone);
  return it == end(ts) ? ts.size() : std::ranges::distance(begin(ts), it);
}

template <std::meta::info scope = std::meta::access_context::current().scope()>
  requires(is_function(scope))
struct DumpArgs {
  static constexpr auto max_idx = parameters_of(scope).size();

  static consteval std::meta::info type(std::size_t idx) {
    if (idx >= max_idx) {
      return ^^Tombstone;
    } else {
      return type_of(parameters_of(scope)[idx]);
    }
  }

  static consteval std::meta::info get(std::size_t idx) {
    if (idx >= max_idx) {
      return std::meta::reflect_constant(tombstone);
    } else {
      return variable_of(parameters_of(scope)[idx]);
    }
  }

  struct result;
  static consteval std::meta::info make_wrapper() { return {}; }
  consteval {
    std::vector<std::meta::info> members;
    for (auto p : parameters_of(scope)) {
      members.push_back(
          data_member_spec(type_of(p), {.name = has_identifier(p) ? identifier_of(p) : "_"}));
    }
    define_aggregate(^^result, members);
  };
  static_assert(is_complete_type(^^result));

  template <typename... Ts>
  static result capture_args(Ts&&... args) {
    return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) -> result {
      return {std::forward<Ts...[Idx]>(args...[Idx])...};
    }(std::make_index_sequence<first_tombstone_idx({remove_cvref(^^Ts)...})>());  
  }
};
}

#define RSL_DUMP_ARGS4(Offset)                                          \
  static_cast<typename[:rsl::_log_impl::DumpArgs<>::type(Offset + 0):]>([:rsl::_log_impl::DumpArgs<>::get(Offset + 0):]), \
  static_cast<typename[:rsl::_log_impl::DumpArgs<>::type(Offset + 1):]>([:rsl::_log_impl::DumpArgs<>::get(Offset + 1):]), \
  static_cast<typename[:rsl::_log_impl::DumpArgs<>::type(Offset + 2):]>([:rsl::_log_impl::DumpArgs<>::get(Offset + 2):]), \
  static_cast<typename[:rsl::_log_impl::DumpArgs<>::type(Offset + 3):]>([:rsl::_log_impl::DumpArgs<>::get(Offset + 3):])
#define RSL_DUMP_ARGS16(Offset) RSL_DUMP_ARGS4(Offset+0), RSL_DUMP_ARGS4(Offset+4), RSL_DUMP_ARGS4(Offset+8), RSL_DUMP_ARGS4(Offset+12)

#ifndef RSL_ARGDUMP_COUNT
#define RSL_ARGDUMP_COUNT 32
#endif
#if RSL_ARGDUMP_COUNT == 64
#define capture_args() rsl::_log_impl::DumpArgs<>::capture_args(RSL_DUMP_ARGS16(0), RSL_DUMP_ARGS16(16), RSL_DUMP_ARGS16(32), RSL_DUMP_ARGS16(48))
#elif RSL_ARGDUMP_COUNT == 32
#define capture_args() rsl::_log_impl::DumpArgs<>::capture_args(RSL_DUMP_ARGS16(0), RSL_DUMP_ARGS16(16))
#elif RSL_ARGDUMP_COUNT == 16
#define capture_args() rsl::_log_impl::DumpArgs<>::capture_args(RSL_DUMP_ARGS16(0))
#elif RSL_ARGDUMP_COUNT == 8
#define capture_args() rsl::_log_impl::DumpArgs<>::capture_args(RSL_DUMP_ARGS4(0), RSL_DUMP_ARGS4(4))
#else
#error "RSL_ARGDUMP_COUNT must be power of 2"
#endif
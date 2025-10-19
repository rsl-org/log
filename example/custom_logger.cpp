#define RSL_DOLLAR_MACROS

#include <rsl/log>
#include <rsl/logging/sinks.hpp>

// struct NullLogger {
//   constexpr static auto min_level = LogLevel::DISABLE;

//   void context(Metadata const& meta, bool entered, bool async_handover) {}

//   template <LogLevel Severity, typename... Args>
//   void emit(Metadata& meta, _impl::FormatString<Severity, Args...> fmt, Args&&... args) {}
// };

template <>
constexpr inline auto rsl::logging::selected_logger<> = rsl::logging::NullLogger();

int main() {
  rsl::warn("global context");
}
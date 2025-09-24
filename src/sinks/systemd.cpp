#include <systemd/sd-journal.h>
#include <rsl/logging/sinks.hpp>
#include <print>

namespace rsl::logging {
namespace {
std::size_t level_to_syslog_level(LogLevel level) {
  switch (level) {
    using enum LogLevel;
    case FATAL: return 2;
    case ERROR: return 3;
    case WARNING: return 4;
    case INFO: return 6;
    case DEBUG: return 7;
    default: return 7;
  }
}

std::string format_name(std::string name, ExtraFields const& arguments) {
  if (not arguments.is_empty()) {
    name = name.substr(0, name.find('('));
    name += "(";
    bool first = true;
    for (auto const& arg : arguments) {
      if (not first) {
        name += ", ";
      } else {
        first = false;
      }
      name += std::format("{} {}={}", arg.type_name(), arg.name, arg.to_repr());
    }
    name += ")";
  }
  return name;
}
}  // namespace

void SystemdSink::emit_event(Event const& event) {
  std::vector<std::string> fields{"CONTAINER=devcontainer"};
  fields.push_back(std::format("PRIORITY={}", level_to_syslog_level(event.meta.severity)));
  fields.push_back("MESSAGE=" + event.text);

  fields.push_back(std::format("CONTEXT_FILE={}", event.meta.context.sloc.file));
  fields.push_back(std::format("CONTEXT_LINE={}", event.meta.context.sloc.line));
  fields.push_back(std::format("CONTEXT_UID={}", event.meta.context.id));
  fields.push_back(std::format("CONTEXT_NAME={}", event.meta.context.name));
  fields.push_back("CONTEXT_FUNC=" + format_name(std::string(event.meta.context.sloc.function), event.meta.context.arguments));

  std::string func_name = format_name(std::string(event.meta.sloc.function), event.meta.arguments);
  for (auto const& arg : event.meta.context.extra) {
    fields.push_back(std::format("{}={}",
                                 arg.name | std::views::transform([](unsigned char c) {
                                   return static_cast<char>(std::toupper(c));
                                 }) | std::ranges::to<std::string>(),
                                 arg.to_string()));
  }

  std::vector<struct iovec> iovecs;
  iovecs.reserve(fields.size());
  for (auto const& field : fields) {
    iovecs.emplace_back((void*)field.c_str(), field.size());
  }

  sd_journal_sendv_with_location(std::format("CODE_FILE={}", event.meta.sloc.file).c_str(),
                                 std::format("CODE_LINE={}", event.meta.sloc.line).c_str(),
                                 func_name.c_str(),
                                 iovecs.data(),
                                 int(iovecs.size()));
}

void SystemdSink::enter_context(Metadata const& meta, bool handover) {}
void SystemdSink::exit_context(Metadata const& meta, bool handover) {}
}  // namespace rsl::logging
#pragma once
#include "logger.hpp"

namespace rsl::loggging {

class TerminalSink final : public Sink {};
using DefaultSink = TerminalSink;

class FileSink final : public Sink{};
class ThreadedFileSink final : public Sink{};

#ifdef __unix__
class SystemdSink final : public Sink{};
#endif
}
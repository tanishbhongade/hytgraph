#pragma once

#include <string_view>

namespace hytgraph::runtime {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
};

void set_log_level(LogLevel level) noexcept;
void log(LogLevel level, std::string_view message);

} // namespace hytgraph::runtime

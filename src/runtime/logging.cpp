#include "runtime/logging.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace hytgraph::runtime {

namespace {
LogLevel g_level = LogLevel::Info;
std::mutex g_mutex;

const char* level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

} // namespace

void set_log_level(LogLevel level) noexcept {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_level = level;
}

void log(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (static_cast<int>(level) < static_cast<int>(g_level)) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif

    std::cerr << '[' << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ") << "] "
              << level_name(level) << " " << message << '\n';
}

} // namespace hytgraph::runtime

#include "runtime/config.hpp"
#include "runtime/logging.hpp"
#include "runtime/result.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>

#ifndef HYTGRAPH_PROJECT_VERSION
#define HYTGRAPH_PROJECT_VERSION "unknown"
#endif

namespace {

std::string utc_timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

void usage(const char* program) {
    std::cout << "Usage: " << program
              << " --output <path> [--experiment-id <id>] [--algorithm <name>]"
                 " [--config key=value]...\n";
}

} // namespace

int main(int argc, char** argv) {
    using namespace hytgraph::runtime;

    std::string output;
    std::string experiment_id = "phase0-dummy";
    Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--experiment-id" && i + 1 < argc) {
            experiment_id = argv[++i];
        } else if (arg == "--algorithm" && i + 1 < argc) {
            config.set("algorithm", argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            const std::string pair = argv[++i];
            const auto pos = pair.find('=');
            if (pos == std::string::npos || pos == 0) {
                std::cerr << "Invalid --config value: " << pair << '\n';
                return 2;
            }
            config.set(pair.substr(0, pos), pair.substr(pos + 1));
        } else if (arg == "--help") {
            usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << '\n';
            usage(argv[0]);
            return 2;
        }
    }

    if (output.empty()) {
        std::cerr << "--output is required\n";
        usage(argv[0]);
        return 2;
    }

    std::filesystem::path output_path(output);
    if (output_path.has_parent_path()) {
        std::filesystem::create_directories(output_path.parent_path());
    }

    log(LogLevel::Info, "running Phase 0 dummy experiment");

    Result result;
    result.experiment_id = experiment_id;
    result.timestamp_utc = utc_timestamp();
    result.config = config.values();
    result.config["project_version"] = HYTGRAPH_PROJECT_VERSION;
    result.measurements["dummy_runtime_seconds"] = 0.0;

    if (!write_result_json(result, output)) {
        log(LogLevel::Error, "failed to write result: " + output);
        return 1;
    }

    log(LogLevel::Info, "wrote structured result: " + output);
    return 0;
}

#include "runtime/config.hpp"
#include "runtime/result.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FAIL: " + message);
}

void test_config() {
    using hytgraph::runtime::Config;
    Config config;
    config.set("algorithm", "pagerank");
    config.set("partition.size_bytes", "33554432");
    config.set("cache.enabled", "true");
    config.set("transfer.gamma", "0.625");

    expect(config.contains("algorithm"), "config contains inserted key");
    expect(config.get("algorithm") == "pagerank", "string lookup");
    expect(config.get_int("partition.size_bytes") == 33554432, "integer conversion");
    expect(config.get_bool("cache.enabled"), "boolean conversion");
    expect(config.get_double("transfer.gamma") == 0.625, "double conversion");
    expect(config.get_or("missing", "fallback") == "fallback", "fallback lookup");
}

void test_result_json() {
    using hytgraph::runtime::Result;
    using hytgraph::runtime::result_to_json;

    Result result;
    result.experiment_id = "unit-test";
    result.timestamp_utc = "2026-01-01T00:00:00Z";
    result.config["algorithm"] = "pagerank";
    result.measurements["runtime_seconds"] = 1.25;

    const std::string json = result_to_json(result);
    expect(json.find("\"schema_version\": \"0.1\"") != std::string::npos,
           "schema version emitted");
    expect(json.find("\"experiment_id\": \"unit-test\"") != std::string::npos,
           "experiment id emitted");
    expect(json.find("\"runtime_seconds\": 1.25") != std::string::npos,
           "measurement emitted");
}

void test_result_file() {
    using hytgraph::runtime::Result;
    using hytgraph::runtime::write_result_json;

    const auto path = std::filesystem::temp_directory_path() / "hytgraph_phase0_test.json";
    Result result;
    result.experiment_id = "file-test";
    result.timestamp_utc = "2026-01-01T00:00:00Z";

    expect(write_result_json(result, path.string()), "result file writes successfully");
    std::ifstream file(path);
    expect(file.good(), "result file exists");
    std::filesystem::remove(path);
}

} // namespace

int main() {
    try {
        test_config();
        test_result_json();
        test_result_file();
        std::cout << "All Phase 0 unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

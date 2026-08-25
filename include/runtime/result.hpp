#pragma once

#include <map>
#include <string>

namespace hytgraph::runtime {

struct Result {
    std::string schema_version = "0.1";
    std::string experiment_id;
    std::string status = "success";
    std::string timestamp_utc;
    std::string git_commit = "unknown";
    std::map<std::string, std::string> config;
    std::map<std::string, double> measurements;
};

[[nodiscard]] std::string result_to_json(const Result& result);
bool write_result_json(const Result& result, const std::string& path);

} // namespace hytgraph::runtime

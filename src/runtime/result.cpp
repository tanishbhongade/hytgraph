#include "runtime/result.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace hytgraph::runtime {

namespace {

std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (const char c : value) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << c; break;
        }
    }
    return out.str();
}

void write_string_map(std::ostringstream& out,
                      const std::map<std::string, std::string>& values) {
    out << '{';
    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first) out << ',';
        first = false;
        out << '\n' << "    \"" << json_escape(key) << "\": \""
            << json_escape(value) << '"';
    }
    if (!values.empty()) out << '\n';
    out << "  }";
}

} // namespace

std::string result_to_json(const Result& result) {
    std::ostringstream out;
    out << std::setprecision(17);
    out << "{\n"
        << "  \"schema_version\": \"" << json_escape(result.schema_version) << "\",\n"
        << "  \"experiment_id\": \"" << json_escape(result.experiment_id) << "\",\n"
        << "  \"status\": \"" << json_escape(result.status) << "\",\n"
        << "  \"timestamp_utc\": \"" << json_escape(result.timestamp_utc) << "\",\n"
        << "  \"git_commit\": \"" << json_escape(result.git_commit) << "\",\n"
        << "  \"config\": ";
    write_string_map(out, result.config);
    out << ",\n  \"measurements\": {";

    bool first = true;
    for (const auto& [key, value] : result.measurements) {
        if (!first) out << ',';
        first = false;
        out << '\n' << "    \"" << json_escape(key) << "\": " << value;
    }
    if (!result.measurements.empty()) out << '\n';
    out << "  }\n}\n";
    return out.str();
}

bool write_result_json(const Result& result, const std::string& path) {
    std::ofstream file(path);
    if (!file) return false;
    file << result_to_json(result);
    return static_cast<bool>(file);
}

} // namespace hytgraph::runtime

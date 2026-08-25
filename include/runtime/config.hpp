#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace hytgraph::runtime {

class Config {
public:
    void set(std::string key, std::string value);

    [[nodiscard]] bool contains(const std::string& key) const;
    [[nodiscard]] const std::string& get(const std::string& key) const;
    [[nodiscard]] std::string get_or(const std::string& key, std::string fallback) const;
    [[nodiscard]] std::int64_t get_int(const std::string& key) const;
    [[nodiscard]] double get_double(const std::string& key) const;
    [[nodiscard]] bool get_bool(const std::string& key) const;

    [[nodiscard]] const std::map<std::string, std::string>& values() const noexcept;

private:
    std::map<std::string, std::string> values_;
};

} // namespace hytgraph::runtime

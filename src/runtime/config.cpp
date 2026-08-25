#include "runtime/config.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace hytgraph::runtime {

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

} // namespace

void Config::set(std::string key, std::string value) {
    if (key.empty()) {
        throw std::invalid_argument("configuration key must not be empty");
    }
    values_[std::move(key)] = std::move(value);
}

bool Config::contains(const std::string& key) const {
    return values_.find(key) != values_.end();
}

const std::string& Config::get(const std::string& key) const {
    const auto it = values_.find(key);
    if (it == values_.end()) {
        throw std::out_of_range("missing configuration key: " + key);
    }
    return it->second;
}

std::string Config::get_or(const std::string& key, std::string fallback) const {
    const auto it = values_.find(key);
    return it == values_.end() ? std::move(fallback) : it->second;
}

std::int64_t Config::get_int(const std::string& key) const {
    return std::stoll(get(key));
}

double Config::get_double(const std::string& key) const {
    return std::stod(get(key));
}

bool Config::get_bool(const std::string& key) const {
    const std::string value = lower(get(key));
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    throw std::invalid_argument("invalid boolean configuration value for key: " + key);
}

const std::map<std::string, std::string>& Config::values() const noexcept {
    return values_;
}

} // namespace hytgraph::runtime

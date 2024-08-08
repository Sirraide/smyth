module;

#include <base/Result.hh>

module smyth.json;

using namespace base;
using namespace smyth;
using namespace smyth::json_utils;

template <>
auto json_utils::Get<std::string>(const json& j) -> Result<const std::string&> {
    if (j.is_string()) return j.get_ref<const std::string&>();
    return Error("Expected string, got '{}'", j.dump());
}

template <>
auto json_utils::Get<json::array_t>(const json& j) -> Result<const json::array_t&> {
    if (j.is_array()) return j.get_ref<const json::array_t&>();
    return Error("Expected array, got '{}'", j.dump());
}

template <>
auto json_utils::Get<json::object_t>(const json& j) -> Result<const json::object_t&> {
    if (j.is_object()) return j.get_ref<const json::object_t&>();
    return Error("Expected object, got '{}'", j.dump());
}

template <>
auto json_utils::Get<bool>(const json& j) -> Result<bool> {
    if (j.is_boolean()) return j.get<bool>();
    return Error("Expected boolean, got '{}'", j.dump());
}

auto json_utils::Parse(std::string_view text) -> Result<json> try {
    return json::parse(text);
} catch (const json::parse_error& e) {
    return Error("Error parsing JSON: {}", e.what());
}

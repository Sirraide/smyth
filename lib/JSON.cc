#include <Smyth/JSON.hh>

using namespace smyth::json_utils;

template <>
auto smyth::json_utils::Get<std::string>(const json& j) -> Result<const std::string&> {
    if (j.is_string()) return j.get_ref<const std::string&>();
    return Error("Expected string, got '{}'", j.dump());
}

template <>
auto smyth::json_utils::Get<json::array_t>(const json& j) -> Result<const json::array_t&> {
    if (j.is_array()) return j.get_ref<const json::array_t&>();
    return Error("Expected array, got '{}'", j.dump());
}

template <>
auto smyth::json_utils::Get<json::object_t>(const json& j) -> Result<const json::object_t&> {
    if (j.is_object()) return j.get_ref<const json::object_t&>();
    return Error("Expected object, got '{}'", j.dump());
}

template <>
auto smyth::json_utils::Get<bool>(const json& j) -> Result<bool> {
    if (j.is_boolean()) return j.get<bool>();
    return Error("Expected boolean, got '{}'", j.dump());
}

auto smyth::json_utils::Parse(std::string_view text) -> Result<nlohmann::json> try {
    return nlohmann::json::parse(text);
} catch (const nlohmann::json::parse_error& e) {
    fmt::print("Invalid JSON:\n{}\n", text);
    return Error("Error parsing JSON: {}", e.what());
}


#ifndef SMYTH_JSON_HH
#define SMYTH_JSON_HH

#include <nlohmann/json.hpp>
#include <Smyth/Utils.hh>

namespace smyth::json_utils {
using json = nlohmann::json;

template <typename Ty>
auto Get(const json& j) -> Result<const Ty&>;

template <typename Ty>
auto Get(json& j) -> Result<Ty&> {
    auto& val = Try(Get<const Ty>(j));
    return const_cast<Ty&>(val);
}

template <std::integral Ty>
auto Get(const json& j) -> Result<Ty> {
    if (j.is_number_integer()) {
        auto val = j.get<Ty>();
        if constexpr (std::unsigned_integral<Ty>) {
            if (val < 0) return Error("Expected unsigned integer, got '{}'", j.dump());
        } else {
            if (val < std::numeric_limits<Ty>::min())
                return Error("Value too small: {}; expected >={}", val, std::numeric_limits<Ty>::min());
        }
        if (val > std::numeric_limits<Ty>::max())
            return Error("Value too large: {}; expected <={}", val, std::numeric_limits<Ty>::max());
        return val;
    }
    return Error("Expected integer, got '{}'", j.dump());
}

template <std::floating_point Ty>
auto Get(const json& j) -> Result<Ty> {
    if (j.is_number_float()) {
        auto val = j.get<Ty>();
        if (std::isnan(val)) return Error("Expected number, got 'NaN'");
        return val;
    }
    return Error("Expected number, got '{}'", j.dump());
}

template <> auto Get<bool>(const json& j) -> Result<bool>;
template <> auto Get<std::string>(const json& j) -> Result<const std::string&>;
template <> auto Get<json::array_t>(const json& j) -> Result<const json::array_t&>;
template <> auto Get<json::object_t>(const json& j) -> Result<const json::object_t&>;

auto Parse(std::string_view text) -> Result<nlohmann::json>;
} // namespace smyth::json_utils

#endif // SMYTH_JSON_HH

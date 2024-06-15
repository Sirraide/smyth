#include <Smyth/Persistent.hh>
#include <algorithm>

using namespace smyth;
using namespace smyth::detail;
using namespace smyth::json_utils;

// FIXME: Window size should be saved in user settings instead here. Then, remove this and priorities.
auto PersistentStore::Entries() {
    std::vector<std::pair<std::string_view, Entry*>> sorted;
    sorted.reserve(entries.size());
    for (auto& [key, entry] : entries) sorted.emplace_back(key, &entry);
    rgs::sort(sorted, [](const auto& a, const auto& b) {
        return a.second->priority < b.second->priority;
    });
    return sorted;
}

auto PersistentStore::load(const json& j) -> Result<> {
    return reload_all(j);
}

void PersistentStore::register_entry(std::string key, Entry entry) {
    Assert(not entries.contains(key), "Duplicate key '{}'", key);
    entries.emplace(std::move(key), std::move(entry));
}

auto PersistentStore::reload_all(const json& j) -> Result<> {
    const json::object_t& obj = Try(Get<json::object_t>(j));
    for (auto& [key, entry] : Entries())
        if (obj.contains(key))
            Try(entry->entry->load(j[key]));
    return {};
}

void PersistentStore::reset_all() {
    for (const auto& [key, entry] : Entries())
        entry->entry->restore();
}

void PersistentStore::restore() {
    reset_all();
}

auto PersistentStore::save() const -> Result<json> {
    return Try(save_all());
}

auto PersistentStore::save_all() const -> Result<json> {
    json j;
    for (const auto& [key, entry] : entries) j[key] = Try(entry.entry->save());
    return j;
}

/// ====================================================================
///  Deserialisers
/// ====================================================================
auto Serialiser<i64>::Deserialise(const json& tn) -> Result<i64> {
    return Get<i64>(tn);
}

auto Serialiser<u64>::Deserialise(const json& tn) -> Result<u64> {
    return Get<u64>(tn);
}

auto Serialiser<bool>::Deserialise(const json& tn) -> Result<bool> {
    return Get<bool>(tn);
}

auto Serialiser<std::string>::Deserialise(const json& tn) -> Result<std::string> {
    return Get<std::string>(tn);
}

/// ====================================================================
///  Serialisers
/// ====================================================================
auto Serialiser<i64>::Serialise(i64 i) -> json {
    return i;
}

auto Serialiser<u64>::Serialise(u64 i) -> json {
    return i;
}

auto Serialiser<bool>::Serialise(bool b) -> json {
    return b;
}

auto Serialiser<std::string>::Serialise(std::string&& val) -> json {
    return std::move(val);
}

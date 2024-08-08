module;

#include <algorithm>
#include <base/Base.hh>
#include <base/Macros.hh>
#include <print>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QList>
#include <QSize>

module smyth.persistent;
import smyth.utils;

using namespace smyth;
using namespace smyth::detail;
using namespace smyth::json_utils;

auto PersistentStore::Create(std::string name, PersistentStore& parent) -> PersistentStore& {
    auto store = new PersistentStore;
    parent.register_entry(
        std::move(name),
        {std::unique_ptr<PersistentBase>{store}, DefaultPriority}
    );
    return *store;
}

// FIXME: Window size should be saved in user settings instead here. Then, remove this and priorities.
auto PersistentStore::Entries() {
    std::vector<std::pair<std::string_view, Entry*>> sorted;
    sorted.reserve(entries.size());
    for (auto& [key, entry] : entries) sorted.emplace_back(key, &entry);
    rgs::stable_sort(sorted, [](const auto& a, const auto& b) {
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
    for (auto& [key, entry] : Entries()) {
        if (obj.contains(key)) {
            auto res = entry->entry->load(j[key]);
            if (not res) std::println(stderr, "Failed to load '{}': {}", key, res.error());
        }
    }
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

auto Serialiser<QByteArray>::Deserialise(const json& j) -> Result<QByteArray> {
    using Status = QByteArray::Base64DecodingStatus;
    auto FormatError = [](Status status) -> std::string_view {
        switch (status) {
            default: return "Unknown error";
            case Status::IllegalCharacter: return "Illegal character";
            case Status::IllegalPadding: return "Illegal padding";
            case Status::IllegalInputLength: return "Illegal input length";
        }
    };

    auto str = Try(Get<std::string>(j));
    auto res = QByteArray::fromBase64Encoding(
        QByteArray::fromStdString(str),
        QByteArray::AbortOnBase64DecodingErrors
    );

    if (res.decodingStatus != Status::Ok) return Error(
        "Failed to decode base64 QByteArray: {}",
        FormatError(res.decodingStatus)
    );

    return std::move(res.decoded);
}

auto Serialiser<QFont>::Deserialise(const json& tn) -> Result<QFont> {
    auto description = Try(Serialiser<QString>::Deserialise(tn));
    QFont f;
    if (not f.fromString(description)) return Error(
        "Invalid font record '{}' in save file",
        description
    );
    return f;
}

auto Serialiser<QSize>::Deserialise(const json& tn) -> Result<QSize> {
    const json::array_t& arr = Try(Get<json::array_t>(tn));
    if (arr.size() != 2) return Error("Expected array of size 2 when reading QSize");
    auto first = Try(Get<i64>(arr.at(0)));
    auto second = Try(Get<i64>(arr.at(1)));
    return QSize{int(first), int(second)};
}

auto Serialiser<QString>::Deserialise(const json& j) -> Result<QString> {
    auto text = Try(Serialiser<std::string>::Deserialise(j));
    return QString::fromUtf8(text.data(), qsizetype(text.size()));
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

auto Serialiser<QByteArray>::Serialise(const QByteArray& val) -> json {
    auto arr = val.toBase64();
    return Serialiser<std::string>::Serialise(arr.toStdString());
}

auto Serialiser<QFont>::Serialise(const QFont& val) -> json {
    return Serialiser<QString>::Serialise(val.toString());
}

auto Serialiser<QString>::Serialise(const QString& val) -> json {
    return Serialiser<std::string>::Serialise(val.toStdString());
}

auto Serialiser<QSize>::Serialise(QSize val) -> json {
    return {val.width(), val.height()};
}

/// ====================================================================
///  Persistence
/// ====================================================================
void smyth::PersistCBox(PersistentStore& store, std::string key, QComboBox* cbox) {
    Persist<&QComboBox::currentIndex, &QComboBox::setCurrentIndex>(
        store,
        std::move(key),
        cbox
    );
}

void smyth::PersistChBox(PersistentStore& store, std::string key, QCheckBox* cbox) {
    Persist<&QCheckBox::checkState, &QCheckBox::setCheckState>(
        store,
        std::move(key),
        cbox
    );
}

/// Like PersistCBox, but the entries are generated dynamically.
void smyth::PersistDynCBox(PersistentStore& store, std::string key, QComboBox* cbox) {
    Persist<&QComboBox::currentText, [](QComboBox* cbox, QString str) {
        // If the item does not exists yet, add it.
        if (cbox->findText(str) == -1) cbox->addItem(str);
        cbox->setCurrentText(str);
    }>(store, std::move(key), cbox);
}

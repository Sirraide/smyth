#ifndef SMYTH_UI_PERSISTOBJECTS_HH
#define SMYTH_UI_PERSISTOBJECTS_HH

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QSize>
#include <QString>
#include <Smyth/Persistent.hh>

namespace smyth::ui {
class App;
}

/// Lists of integers.
///
/// Note that we don’t just allow storing arbitrary lists as the
/// objects contained within them may change size, which would cause
/// deserialisation to fail; integers and enums are always encoded
/// as 64-bit numbers, so they’re safe from that.
template <typename Int>
requires std::integral<Int> or std::is_enum_v<Int>
struct smyth::detail::Serialiser<QList<Int>> {
    static_assert(sizeof(Int) <= sizeof(i64), "Integer or enum too large");
    static auto Deserialise(const json_utils::json& j) -> Result<QList<Int>> {
        const json_utils::json::array_t& arr = Try(json_utils::Get<json_utils::json::array_t>(j));
        QList<Int> result;
        for (const auto& item : arr)
            result.push_back(Try(Serialiser<Int>::Deserialise(item)));
        return std::move(result);
    }

    static auto Serialise(const QList<Int>& list) -> json_utils::json {
        json_utils::json::array_t arr;
        for (auto item : list) arr.push_back(item);
        return std::move(arr);
    }
};

SMYTH_DECLARE_SERIALISER(const QByteArray&, QByteArray);
SMYTH_DECLARE_SERIALISER(const QFont&, QFont);
SMYTH_DECLARE_SERIALISER(QSize, QSize);
SMYTH_DECLARE_SERIALISER(const QString&, QString);

namespace smyth::ui {
void PersistCBox(PersistentStore& store, std::string key, QComboBox* cbox);
void PersistChBox(PersistentStore& store, std::string key, QCheckBox* cbox);

/// Like PersistCBox, but the entries are generated dynamically.
void PersistDynCBox(PersistentStore& store, std::string key, QComboBox* cbox);

/// Persist an object’s state as base64.
template <typename Object>
void PersistState(PersistentStore& store, std::string key, Object* obj) {
    Persist<&Object::saveState, &Object::restoreState>(store, std::move(key), obj);
}
} // namespace smyth::ui

#endif // SMYTH_UI_PERSISTOBJECTS_HH

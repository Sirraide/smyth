#ifndef SMYTH_UI_PERSISTOBJECTS_HH
#define SMYTH_UI_PERSISTOBJECTS_HH

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QSize>
#include <QSplitter>
#include <QString>
#include <Smyth/Persistent.hh>

namespace smyth::ui {
class App;
}

namespace smyth::ui::detail {
template <typename>
struct Serialiser;

template <typename T>
using MakeResult = std::conditional_t<IsResult<T>, T, Result<T>>;

/// Helper to persist a ‘property’ of an object.
template <typename RawType, typename Object, auto Get, auto Set>
class PersistProperty : public smyth::detail::PersistentBase {
    using Type = std::decay_t<RawType>;
    Object* obj;
    Type default_value;

public:
    PersistProperty(Object* obj)
        : obj(obj),
          default_value(std::invoke(Get, obj)) {}

private:
    auto Deserialise(Column c) -> MakeResult<decltype(Serialiser<Type>::Deserialise(c))> {
        return Serialiser<Type>::Deserialise(c);
    }

    auto load(Column c) -> Result<> override {
        auto stored = Try(Deserialise(c));
        std::invoke(Set, obj, std::move(stored));
        return {};
    }

    auto modified(Column c) -> Result<bool> override {
        auto stored = Try(Deserialise(c));
        return std::invoke(Get, obj) != stored;
    }

    void restore() override {
        std::invoke(Set, obj, default_value);
    }

    auto save(QueryParamRef q) -> Result<> override {
        Serialiser<Type>::Serialise(q, std::invoke(Get, obj));
        return {};
    }
};

template <>
struct Serialiser<QString> {
    static auto Deserialise(Column c) -> QString {
        auto text = c.text();
        return QString::fromUtf8(text.data(), qsizetype(text.size()));
    }

    static void Serialise(QueryParamRef q, QString s) {
        auto arr = s.toUtf8();
        q.bind(std::string_view{arr.data(), usz(arr.size())});
    }
};

template <>
struct Serialiser<QSize> {
    static auto Deserialise(Column c) -> QSize {
        // 32 lower bits are width, 32 upper bits are height.
        auto encoded = c.integer();
        return QSize{static_cast<int>(encoded & 0xFFFF'FFFF), static_cast<int>(encoded >> 32)};
    }

    static void Serialise(QueryParamRef q, QSize s) {
        // 32 lower bits are width, 32 upper bits are height.
        auto encoded = static_cast<u64>(s.width()) | (static_cast<u64>(s.height()) << 32);
        q.bind(encoded);
    }
};

template <typename I>
requires std::integral<I> or std::is_enum_v<I>
struct Serialiser<I> {
    static auto Deserialise(Column c) -> I {
        return static_cast<I>(c.integer());
    }

    static void Serialise(QueryParamRef q, I i) {
        q.bind(static_cast<i64>(i));
    }
};

template <>
struct Serialiser<QFont> {
    static auto Deserialise(Column c) -> Result<QFont> {
        auto description = c.text();
        QFont f;
        if (not f.fromString(QString::fromStdString(description))) return Err(
            "Invalid font record '{}' in save file",
            description
        );
        return f;
    }

    static void Serialise(QueryParamRef q, const QFont& font) {
        q.bind(font.toString().toStdString());
    }
};

/// Lists of integers.
///
/// Note that we don’t just allow storing arbitrary lists as the
/// objects contained within them may change size, which would cause
/// deserialisation to fail; integers and enums are always encoded
/// as 64-bit numbers, so they’re safe from that.
template <typename Int>
requires std::integral<Int> or std::is_enum_v<Int>
struct Serialiser<QList<Int>> {
    static_assert(sizeof(Int) <= sizeof(i64), "Integer or enum too large");
    static auto Deserialise(Column c) -> Result<QList<Int>> {
        QList<Int> result;
        const auto blob = c.blob();
        const auto count = blob.size() / sizeof(i64);
        result.reserve(qsizetype(count));
        for (usz i = 0; i < count; ++i) {
            i64 item;
            std::memcpy(&item, blob.data() + i * sizeof(i64), sizeof(i64));
            result.push_back(Int(item));
        }
        return result;
    }

    static void Serialise(QueryParamRef q, const QList<Int>& list) {
        std::vector<std::byte> blob;
        const auto size = usz(list.size());
        blob.resize(size * sizeof(i64));
        for (usz i = 0; i < size; ++i) {
            i64 item = i64(list[qsizetype(i)]);
            std::memcpy(blob.data() + i * sizeof(i64), &item, sizeof(i64));
        }
        q.bind(std::span{blob});
    }
};
} // namespace smyth::ui::detail

namespace smyth::ui {
void PersistCBox(std::string key, QComboBox* cbox);
void PersistChBox(std::string key, QCheckBox* cbox);

/// Like PersistCBox, but the entries are generated dynamically.
void PersistDynCBox(std::string key, QComboBox* cbox);

/// Splitters may crash if we supply a value that is larger than the total width.
void PersistSplitter(std::string key, QSplitter* splitter);
}

#endif // SMYTH_UI_PERSISTOBJECTS_HH

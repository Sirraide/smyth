#ifndef SMYTH_PERSISTOBJECTS_HH
#define SMYTH_PERSISTOBJECTS_HH

#include <bit>
#include <glaze/glaze.hpp>
#include <Persistent.hh>
#include <QFont>
#include <QSize>
#include <QString>

namespace smyth::detail {
template <typename>
struct Serialiser;

/// Helper to persist a ‘property’ of an object.
template <typename Type, typename Object, auto Get, auto Set>
class PersistProperty : public PersistentBase {
    Object* obj;

public:
    PersistProperty(Object* obj) : obj(obj) {}

private:
    void load(Column c) override {
        auto stored = Serialiser<Type>::Deserialise(c);
        if (not stored.has_value()) return;
        std::invoke(Set, obj, std::move(*stored));
    }

    void save(QueryParamRef q) override {
        return Serialiser<Type>::Serialise(q, std::invoke(Get, obj));
    }
};

/// The serialisation format deliberately avoids storing raw bytes
/// in most cases so we don’t run into en

template <>
struct Serialiser<QString> {
    static auto Deserialise(Column c) -> std::optional<QString> {
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
    static auto Deserialise(Column c) -> std::optional<QSize> {
        /// 32 lower bits are width, 32 upper bits are height.
        auto encoded = c.integer();
        return QSize{static_cast<int>(encoded & 0xFFFF'FFFF), static_cast<int>(encoded >> 32)};
    }

    static void Serialise(QueryParamRef q, QSize s) {
        /// 32 lower bits are width, 32 upper bits are height.
        auto encoded = static_cast<u64>(s.width()) | (static_cast<u64>(s.height()) << 32);
        q.bind(encoded);
    }
};

template <std::integral I>
struct Serialiser<I> {
    static auto Deserialise(Column c) -> std::optional<I> {
        return static_cast<I>(c.integer());
    }

    static void Serialise(QueryParamRef q, I i) {
        q.bind(static_cast<i64>(i));
    }
};

template <>
struct Serialiser<const QFont&> {
    static auto Deserialise(Column c) -> std::optional<QFont> {
        auto description = c.text();
        QFont f;
        if (not f.fromString(QString::fromStdString(description))) return std::nullopt;
        return f;
    }

    static void Serialise(QueryParamRef q, const QFont& font) {
        q.bind(font.toString().toStdString());
    }
};

template <typename Internal>
requires std::is_trivially_constructible_v<Internal> /// For memcpy().
struct Serialiser<QList<Internal>> {
    static auto Deserialise(Column c) -> std::optional<QList<Internal>> {
        QList<Internal> result;
        auto blob = c.blob();
        if (glz::read_binary_untagged(result, blob)) {
            Error("Failed to deserialise {}", glz::name_v<QList<Internal>>);
            return std::nullopt;
        }

        return result;
    }

    static void Serialise(QueryParamRef q, const QList<Internal>& list) {
        std::vector<std::byte> blob;
        glz::write_binary_untagged(std::span<const Internal>{list.data(), usz(list.size())}, blob);
        q.bind(std::span{blob});
    }
};

} // namespace smyth::detail

#endif // SMYTH_PERSISTOBJECTS_HH

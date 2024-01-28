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

template <typename T>
using MakeResult = std::conditional_t<IsResult<T>, T, Result<T>>;

/// Helper to persist a ‘property’ of an object.
template <typename RawType, typename Object, auto Get, auto Set>
class PersistProperty : public PersistentBase {
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

    void reset() override {
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

template <typename Internal>
requires std::is_trivially_constructible_v<Internal> /// For memcpy().
struct Serialiser<QList<Internal>> {
    static auto Deserialise(Column c) -> Result<QList<Internal>> {
        QList<Internal> result;
        auto blob = c.blob();
        if (glz::read_binary_untagged(result, blob) == 0) return result;
        else return Err("Failed to deserialise {}", glz::name_v<QList<Internal>>);
    }

    static void Serialise(QueryParamRef q, const QList<Internal>& list) {
        fmt::print("Serialising [{}]\n", fmt::join(list, ", "));
        std::vector<std::byte> blob;
        glz::write_binary_untagged(std::span<const Internal>{list.data(), usz(list.size())}, blob);
        q.bind(std::span{blob});
    }
};

} // namespace smyth::detail

#endif // SMYTH_PERSISTOBJECTS_HH

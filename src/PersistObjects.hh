#ifndef SMYTH_PERSISTOBJECTS_HH
#define SMYTH_PERSISTOBJECTS_HH

#include <Persistent.hh>
#include <QString>
#include <QSize>
#include <bit>

namespace smyth::detail {
template <typename>
struct Serialiser;

/// Helper to persist a ‘property’ of an object.
template <typename Type, typename Object, auto Get, auto Set>
class PersistProperty : public PersistentBase {
    Object* obj;

public:
    PersistProperty (Object* obj) : obj(obj) {}

private:
    void load(Column c) override {
        std::invoke(Set, obj, Serialiser<Type>::Deserialise(c));
    }

    void save(QueryParamRef q) override {
        return Serialiser<Type>::Serialise(q, std::invoke(Get, obj));
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
        return QSize{static_cast<int>(encoded & 0xFFFFFFFF),
                     static_cast<int>(encoded >> 32)};
    }

    static void Serialise(QueryParamRef q, QSize s) {
        /// 32 lower bits are width, 32 upper bits are height.
        auto encoded = static_cast<u64>(s.width()) | (static_cast<u64>(s.height()) << 32);
        q.bind(encoded);
    }

};

} // namespace smyth::detail

#endif // SMYTH_PERSISTOBJECTS_HH

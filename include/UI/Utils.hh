#ifndef SMYTH_UI_UTILS_HH
#define SMYTH_UI_UTILS_HH

#include <fmt/format.h>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

#define SMYTH_MAIN_STORE_KEY "store"

namespace smyth::ui {
class MainWindow;
class SettingsDialog;

namespace detail {
template <typename>
struct ExtractTypeImpl;

template <typename Type, typename Object>
struct ExtractTypeImpl<Type(Object::*)> {
    using type = Type;
};

template <typename Type, typename Object>
struct ExtractTypeImpl<Type (Object::*)() const> {
    using type = Type;
};

template <typename Type>
using ExtractType = typename ExtractTypeImpl<Type>::type;
} // namespace detail
} // namespace smyth::ui

template <>
struct fmt::formatter<QString> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const QString& s, FormatContext& ctx) {
        return formatter<std::string>::format(s.toStdString(), ctx);
    }
};

template <>
struct fmt::formatter<QRect> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const QRect& r, FormatContext& ctx) {
        return formatter<std::string_view>::format(
            fmt::format("[x: {}, y: {}, wd: {}, ht: {}]", r.x(), r.y(), r.width(), r.height()),
            ctx
        );
    }
};

template <>
struct fmt::formatter<QSize> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(QSize s, FormatContext& ctx) {
        return formatter<std::string_view>::format(
            fmt::format("({}, {})", s.width(), s.height()),
            ctx
        );
    }
};

template <>
struct fmt::formatter<QPointF> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(QPointF s, FormatContext& ctx) {
        return formatter<std::string_view>::format(
            fmt::format("({}, {})", s.x(), s.y()),
            ctx
        );
    }
};

template <typename T>
struct fmt::formatter<QList<T>> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const QList<T> s, FormatContext& ctx) {
        return formatter<std::string_view>::format(
            fmt::format("[{}]", fmt::join(s, ", ")),
            ctx
        );
    }
};

#endif // SMYTH_UI_UTILS_HH

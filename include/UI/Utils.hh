#ifndef SMYTH_UI_UTILS_HH
#define SMYTH_UI_UTILS_HH

#include <fmt/format.h>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

namespace smyth::ui {
class MainWindow;
class SettingsDialog;
}

template <>
struct fmt::formatter<QString> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const QString& s, FormatContext& ctx) {
        return formatter<std::string>::format(s.toStdString(), ctx);
    }
};

template <>
struct fmt::formatter<QAnyStringView> : formatter<std::string> {
    template <typename FormatContext>
    auto format(QAnyStringView s, FormatContext& ctx) {
        return formatter<std::string>::format(s.toString().toStdString(), ctx);
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

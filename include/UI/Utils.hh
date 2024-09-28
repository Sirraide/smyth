#ifndef SMYTH_UI_UTILS_HH
#define SMYTH_UI_UTILS_HH

#include <QFont>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVariant>

namespace smyth::ui {
class MainWindow;
class SettingsDialog;
}

template <>
struct std::formatter<QString> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const QString& s, FormatContext& ctx) const {
        return formatter<std::string>::format(s.toStdString(), ctx);
    }
};

template <>
struct std::formatter<QAnyStringView> : formatter<std::string> {
    template <typename FormatContext>
    auto format(QAnyStringView s, FormatContext& ctx) const {
        return formatter<std::string>::format(s.toString().toStdString(), ctx);
    }
};

template <>
struct std::formatter<QRect> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const QRect& r, FormatContext& ctx) const {
        return formatter<std::string_view>::format(
            std::format("[x: {}, y: {}, wd: {}, ht: {}]", r.x(), r.y(), r.width(), r.height()),
            ctx
        );
    }
};

template <>
struct std::formatter<QSize> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(QSize s, FormatContext& ctx) const {
        return formatter<std::string_view>::format(
            std::format("({}, {})", s.width(), s.height()),
            ctx
        );
    }
};

template <>
struct std::formatter<QPointF> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(QPointF s, FormatContext& ctx) const {
        return formatter<std::string_view>::format(
            std::format("({}, {})", s.x(), s.y()),
            ctx
        );
    }
};

template <typename T>
struct std::formatter<QList<T>> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const QList<T>& list, FormatContext& ctx) const {
        std::string s{"["};
        for (const auto& item : list) s += std::format("{}, ", item);
        if (not s.empty()) s.pop_back();
        if (not s.empty()) s.pop_back();
        s += "]";
        return formatter<std::string_view>::format(s, ctx);
    }
};

template <>
struct std::formatter<QVariant> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const QVariant& v, FormatContext& ctx) const {
        std::string s{v.toString().toStdString()};
        return formatter<std::string_view>::format(s, ctx);
    }
};

template <>
struct std::formatter<QFont> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const QFont& f, FormatContext& ctx) const {
        return formatter<std::string_view>::format(
            std::format("{}:{}", f.family().toStdString(), f.pointSize()),
            ctx
        );
    }
};

#endif // SMYTH_UI_UTILS_HH

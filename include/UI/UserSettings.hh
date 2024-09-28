#ifndef SMYTH_UI_USER_SETTINGS_HH
#define SMYTH_UI_USER_SETTINGS_HH

#include <functional>
#include <map>
#include <QSettings>
#include <QVariant>
#include <Smyth/Utils.hh>
#include <UI/Utils.hh>

namespace smyth::ui {
template <typename = QString>
class UserSetting;

namespace detail::user_settings {
void Init();
class SettingsBase {
    LIBBASE_IMMOVABLE(SettingsBase);
    friend void Init();

protected:
    virtual ~SettingsBase() = default;
    SettingsBase() = default;

private:
    virtual void init(QVariant value) = 0;
};

extern std::map<QAnyStringView, SettingsBase*> All;
} // namespace detail::user_settings
} // namespace smyth::ui

/// Simplified version of the persistence API that relies on
/// Qt’s builtin settings mechanism. Used for global per-user
/// settings.
template <typename Type>
class smyth::ui::UserSetting final : public detail::user_settings::SettingsBase {
    std::vector<std::function<void(const Type&)>> subscribers;
    QString key;
    Type cached_value;

public:
    /// Create a new setting with the specified key.
    explicit UserSetting(QString k, Type default_value)
        : key{std::move(k)},
          cached_value{std::move(default_value)} {
        Assert(not detail::user_settings::All.contains(key), "Duplicate settings key '{}'", key);
        detail::user_settings::All.emplace(std::move(key), this);
    }

    /// Get the value of this setting.
    [[nodiscard]] auto operator*() const -> const Type& {
        return cached_value;
    }

    /// Update the value of this setting.
    void set(const Type& val) {
        // Only update if the value has changed.
        if (cached_value == val) return;

        // Dew it.
        cached_value = val;
        QSettings settings{QSettings::UserScope};
        settings.setValue(key, QVariant::fromValue(val));
        for (const auto& sub : subscribers) sub(val);
    }

    /// Subscribe to changes to this setting.
    void subscribe(std::function<void(const Type&)> cb) {
        subscribers.push_back(std::move(cb));
    }

    /// Subscribe to call a function when the setting changes.
    template <typename Object, typename ThisType>
    void subscribe(Object* o, void (ThisType::*cb)(const Type&)) {
        subscribe([o, cb](const Type& val) { (o->*cb)(val); });
    }

    /// Subscribe to call a function when the setting changes.
    template <typename Object, typename ThisType>
    void subscribe(Object* o, void (ThisType::*cb)(Type)) {
        subscribe([o, cb](const Type& val) { (o->*cb)(val); });
    }

private:
    void init(QVariant value) override {
        if (value.isValid() and value.canConvert<Type>()) set(value.value<Type>());
    }
};

/// Per-user settings.
namespace smyth::ui::settings {
extern UserSetting<QFont> MonoFont;
extern UserSetting<QFont> SerifFont;
extern UserSetting<QFont> SansFont;
extern UserSetting<> LastOpenProject;

#ifdef LIBBASE_DEBUG
extern UserSetting<bool> DumpJsonRequests;
#endif
} // namespace settings

#endif // SMYTH_UI_USER_SETTINGS_HH

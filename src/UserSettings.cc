#include <UI/UserSettings.hh>

using namespace smyth::ui;
using namespace smyth::ui::detail;

// This needs to be created *before* any of the settings.
std::map<QAnyStringView, user_settings::SettingsBase*> user_settings::All;

/// Per-user settings.
UserSetting<QFont> settings::MonoFont{"mono.font", QFont{"monospace"}};
UserSetting<QFont> settings::SerifFont{"serif.font", QFont{"serif"}};
UserSetting<QFont> settings::SansFont{"sans.font", QFont{"sans"}};
UserSetting<> settings::LastOpenProject{"last_open_project", ""};

#ifdef LIBBASE_DEBUG
UserSetting<bool> settings::DumpJsonRequests{"__debug__/dump_json_requests", false};
#endif

void user_settings::Init() {
    QSettings settings{QSettings::UserScope};
    for (auto [key, setting] : All) setting->init(settings.value(key));
}

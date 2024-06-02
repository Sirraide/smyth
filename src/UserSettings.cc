#include <UI/UserSettings.hh>

using namespace smyth::ui::detail;

std::map<QAnyStringView, user_settings::SettingsBase*> user_settings::All;
void user_settings::Init() {
    QSettings settings{QSettings::UserScope};
    for (auto [key, setting] : All) setting->init(settings.value(key));
}
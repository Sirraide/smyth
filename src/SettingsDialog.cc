#include <base/Numeric.hh>
#include <base/Stream.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/SmythDictionary.hh>
#include <ui_SettingsDialog.h>

using namespace smyth;
using namespace smyth::ui;

SettingsDialog* SettingsDialog::Instance;

// ====================================================================
//  Initialisation
// ====================================================================
SettingsDialog::SettingsDialog(MainWindow* main)
    : QDialog(main), ui(std::make_unique<Ui::SettingsDialog>()) {
    ui->setupUi(this);
    connect(
        ui->debug_show_json,
        &QCheckBox::toggled,
        this,
        &SettingsDialog::toggle_show_json_requests
    );
}

void SettingsDialog::Init() {
#ifdef LIBBASE_DEBUG
    settings::DumpJsonRequests.subscribe([this](bool checked) {
        ui->debug_show_json->setChecked(checked);
    });
#endif

    Persist();
}

void SettingsDialog::Persist() {
    auto& store = PersistentStore::Create("settings");
    Persist<&QLineEdit::text, &QLineEdit::setText>(
        store,
        "duplicate_rows",
        ui->text_duplicate_rows
    );
}

// ====================================================================
//  API
// ====================================================================
SettingsDialog::~SettingsDialog() noexcept = default;

void SettingsDialog::Exec() {
    Instance->exec();
}

auto SettingsDialog::GetRowsToDuplicate() -> Result<QList<int>> {
    QList<int> indices;
    auto text = Instance->ui->text_duplicate_rows->text().trimmed().toStdString();
    stream s{text};
    for (;;) {
        s.trim_front();
        if (s.empty()) break;
        auto number = stream{s.take_until(',')}.trim().text();
        auto val = Try(Parse<int>(number));
        if (not indices.contains(val)) indices.push_back(val);
        s.drop(1);
    }
    return indices;
}

void SettingsDialog::Reset() {
    // No-op. Add project-specific settings here if need be.
}

// ====================================================================
//  Slots
// ====================================================================
static void SetFont(UserSetting<QFont>& setting, QFontComboBox* from_cbox) {
    QFont font{*setting};
    font.setFamily(from_cbox->currentFont().family());
    setting.set(font);
}

void SettingsDialog::set_default_font() {
    SetFont(settings::SerifFont, ui->font_default);
}

void SettingsDialog::set_mono_font() {
    SetFont(settings::MonoFont, ui->font_mono);
}

void SettingsDialog::set_notes_font() {
    SetFont(settings::SansFont, ui->font_notes);
}

void SettingsDialog::toggle_show_json_requests() {
#ifdef LIBBASE_DEBUG
    settings::DumpJsonRequests.set(ui->debug_show_json->isChecked());
#endif
}

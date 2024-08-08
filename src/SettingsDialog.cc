#include <base/Numeric.hh>
#include <base/Stream.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/SmythDictionary.hh>
#include <ui_SettingsDialog.h>

import smyth.persistent;

using namespace smyth;
using namespace smyth::ui;

SettingsDialog::~SettingsDialog() noexcept = default;

SettingsDialog::SettingsDialog()
    : QDialog(App::MainWindow()), ui(std::make_unique<Ui::SettingsDialog>()) {
    ui->setupUi(this);

    connect(
        ui->debug_show_json,
        &QCheckBox::toggled,
        this,
        &SettingsDialog::toggle_show_json_requests
    );
}

auto SettingsDialog::get_rows_to_duplicate() const -> Result<QList<int>> {
    QList<int> indices;
    auto text = ui->text_duplicate_rows->text().trimmed().toStdString();
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

void SettingsDialog::init() {
#ifdef LIBBASE_DEBUG
    App::The().dump_json_requests.subscribe([this](bool checked) {
        ui->debug_show_json->setChecked(checked);
    });
#endif

    auto& store = PersistentStore::Create("settings");
    persist(&store);
}

void SettingsDialog::persist(void* store) {
    Persist<&QLineEdit::text, &QLineEdit::setText>(
        *static_cast<PersistentStore*>(store),
        "duplicate_rows",
        ui->text_duplicate_rows
    );
}

void SettingsDialog::reset_dialog() {
    // No-op. Add project-specific settings here if need be.
}

void SettingsDialog::set_default_font() {
    QFont font{*App::The().serif_font};
    font.setFamily(ui->font_default->currentFont().family());
    App::The().serif_font.set(font);
}

void SettingsDialog::set_mono_font() {
    QFont font{*App::The().mono_font};
    font.setFamily(ui->font_mono->currentFont().family());
    App::The().mono_font.set(font);
}

void SettingsDialog::set_notes_font() {
    QFont font{*App::The().sans_font};
    font.setFamily(ui->font_notes->currentFont().family());
    App::The().sans_font.set(font);
}

void SettingsDialog::toggle_show_json_requests() {
#ifdef LIBBASE_DEBUG
    App::The().dump_json_requests.set(ui->debug_show_json->isChecked());
#endif
}

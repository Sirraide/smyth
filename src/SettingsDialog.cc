#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <ui_SettingsDialog.h>

smyth::ui::SettingsDialog::~SettingsDialog() noexcept = default;

smyth::ui::SettingsDialog::SettingsDialog()
    : QDialog(App::MainWindow()), ui(std::make_unique<Ui::SettingsDialog>()) {
    ui->setupUi(this);

    connect(
        ui->debug_show_json,
        &QCheckBox::toggled,
        this,
        &SettingsDialog::toggle_show_json_requests
    );
}

void smyth::ui::SettingsDialog::init() {
#ifdef LIBBASE_DEBUG
    App::The().dump_json_requests.subscribe([this](bool checked) {
        ui->debug_show_json->setChecked(checked);
    });
#endif
}

void smyth::ui::SettingsDialog::reset_dialog() {
    // No-op. Add project-specific settings here if need be.
}

void smyth::ui::SettingsDialog::set_default_font() {
    QFont font{*App::The().serif_font};
    font.setFamily(ui->font_default->currentFont().family());
    App::The().serif_font.set(font);
}

void smyth::ui::SettingsDialog::set_mono_font() {
    QFont font{*App::The().mono_font};
    font.setFamily(ui->font_mono->currentFont().family());
    App::The().mono_font.set(font);
}

void smyth::ui::SettingsDialog::toggle_show_json_requests() {
#ifdef LIBBASE_DEBUG
    App::The().dump_json_requests.set(ui->debug_show_json->isChecked());
#endif
}

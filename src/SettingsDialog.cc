#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <ui_SettingsDialog.h>

smyth::ui::SettingsDialog::~SettingsDialog() noexcept = default;

smyth::ui::SettingsDialog::SettingsDialog()
    : QDialog(App::MainWindow()), ui(std::make_unique<Ui::SettingsDialog>()) {
    ui->setupUi(this);
}

void smyth::ui::SettingsDialog::init() {
#ifdef SMYTH_DEBUG
    PersistChBox(App::The().debug_store, "settings_dump_json_requests", ui->debug_show_json);
#endif
}

void smyth::ui::SettingsDialog::reset_dialog() {
    // Init font names.
    ui->font_default->setCurrentFont(App::MainWindow()->serif_font());
    ui->font_mono->setCurrentFont(App::MainWindow()->mono_font());
}

void smyth::ui::SettingsDialog::set_default_font() {
    App::MainWindow()->set_serif_font(ui->font_default->currentFont());
}

void smyth::ui::SettingsDialog::set_mono_font() {
    App::MainWindow()->set_mono_font(ui->font_mono->currentFont());
}

bool smyth::ui::SettingsDialog::show_json_requests() const {
    return ui->debug_show_json->isChecked();
}

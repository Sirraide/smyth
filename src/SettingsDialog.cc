#include <SettingsDialog.hh>
#include <MainWindow.hh>
#include <ui_SettingsDialog.h>

smyth::SettingsDialog::~SettingsDialog() noexcept = default;

smyth::SettingsDialog::SettingsDialog(App& app)
    : QDialog(app.main_window()), app(app), ui(std::make_unique<Ui::SettingsDialog>()) {
    ui->setupUi(this);

    /// Init font names.
    ui->font_default->setCurrentFont(app.main_window()->serif_font());
    ui->font_mono->setCurrentFont(app.main_window()->mono_font());
}

void smyth::SettingsDialog::set_default_font() {
    app.main_window()->set_serif_font(ui->font_default->currentFont());
}

void smyth::SettingsDialog::set_mono_font() {
    app.main_window()->set_mono_font(ui->font_mono->currentFont());
}
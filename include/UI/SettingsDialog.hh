#ifndef SMYTH_UI_SETTINGSDIALOG_HH
#define SMYTH_UI_SETTINGSDIALOG_HH

#include <QDialog>
#include <UI/App.hh>

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsDialog;
}
QT_END_NAMESPACE

namespace smyth::ui {
class SettingsDialog final : public QDialog {
    Q_OBJECT

    std::unique_ptr<Ui::SettingsDialog> ui;

public:
    SMYTH_IMMOVABLE(SettingsDialog);
    SettingsDialog();
    ~SettingsDialog() noexcept;

    /// Reset the settings dialog as appropriate for a new project. This
    /// must be called after the main window has been initialised.
    ///
    /// This is not just called `reset()` because I’ve accidentally called
    /// `std::unique_ptr::reset()` on this thing before...
    void reset_dialog();

public slots:
    void set_default_font();
    void set_mono_font();
};
} // namespace smyth::ui

#endif // SMYTH_UI_SETTINGSDIALOG_HH

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
class SettingsDialog;
} // namespace smyth::ui

class smyth::ui::SettingsDialog final : public QDialog {
    Q_OBJECT

    std::unique_ptr<Ui::SettingsDialog> ui;

public:
    LIBBASE_IMMOVABLE(SettingsDialog);
    SettingsDialog();
    ~SettingsDialog() noexcept;

    /// Get the rows in the dictionary we should duplicate.
    auto get_rows_to_duplicate() const -> Result<QList<int>>;

    /// Initialise the dialog.
    void init();

    /// Persist settings.
    void persist(void* store); // FIXME(Modularise)

    /// Reset the settings dialog as appropriate for a new project. This
    /// must be called after the main window has been initialised.
    ///
    /// This is not just called `reset()` because I’ve accidentally called
    /// `std::unique_ptr::reset()` on this thing before...
    void reset_dialog();

public slots:
    void set_default_font();
    void set_mono_font();
    void set_notes_font();
    void toggle_show_json_requests();
};

#endif // SMYTH_UI_SETTINGSDIALOG_HH

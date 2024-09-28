#ifndef SMYTH_UI_SETTINGSDIALOG_HH
#define SMYTH_UI_SETTINGSDIALOG_HH

#include <QDialog>
#include <UI/Smyth.hh>

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
    LIBBASE_IMMOVABLE(SettingsDialog);
    friend void ui::InitialiseSmyth();

    std::unique_ptr<Ui::SettingsDialog> ui;

    /// The settings dialog instance. There is only one.
    static SettingsDialog* Instance;

    SettingsDialog(MainWindow* main);

public:
    ~SettingsDialog() noexcept override;

    /// Open the dialog.
    static void Exec();

    /// Get the rows in the dictionary we should duplicate.
    static auto GetRowsToDuplicate() -> Result<QList<int>>;

    /// Reset the settings dialog as appropriate for a new project. This
    /// must be called after the main window has been initialised.
    static void Reset();

public slots:
    void set_default_font();
    void set_mono_font();
    void set_notes_font();
    void toggle_show_json_requests();

private:
    void Init();
    void Persist();
};

#endif // SMYTH_UI_SETTINGSDIALOG_HH

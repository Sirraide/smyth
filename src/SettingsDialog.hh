#ifndef SMYTH_SETTINGSDIALOG_HH
#define SMYTH_SETTINGSDIALOG_HH

#include <App.hh>
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsDialog;
}
QT_END_NAMESPACE

namespace smyth {
class SettingsDialog : public QDialog {
    Q_OBJECT

    smyth::App& app;
    std::unique_ptr<Ui::SettingsDialog> ui;

public:
    SMYTH_IMMOVABLE(SettingsDialog);
    SettingsDialog(App& app);
    ~SettingsDialog() noexcept;

public slots:
    void set_default_font();
    void set_mono_font();
};
} // namespace smyth

#endif // SMYTH_SETTINGSDIALOG_HH

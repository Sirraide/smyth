#ifndef SMYTH_MAINWINDOW_HH
#define SMYTH_MAINWINDOW_HH

#include <App.hh>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace smyth {
class SettingsDialog;
class MainWindow : public QMainWindow {
    Q_OBJECT

    smyth::App& app;
    std::unique_ptr<Ui::MainWindow> ui;

    /// App is the main controller of the program and needs to call protected
    /// functions sometimes.
    friend smyth::App;

public:
    SMYTH_IMMOVABLE(MainWindow);
    MainWindow(App& app);
    ~MainWindow() noexcept;

    /// Handle a user pressing the close button.
    void closeEvent(QCloseEvent *event) override;

    /// Initialise persistent objects.
    void persist();

    /// Get the current default monospace font.
    auto mono_font() const -> QFont;

    /// Get the current default serif font.
    auto serif_font() const -> QFont;

    /// Set the default monospace font.
    void set_mono_font(QFont f);

    /// Set the default serif font.
    void set_serif_font(QFont f);

public slots:
    void apply_sound_changes();
    void new_project();
    void open_project();
    void open_settings();
    void preview_changes_after_eval();
    void save_project();

private:
    auto ApplySoundChanges() -> Result<>;
    auto EvaluateAndInterpolateJavaScript(QString& in_string) -> Result<>;
    void HandleErrors(Result<> r);
};
} // namespace smyth
#endif // SMYTH_MAINWINDOW_HH

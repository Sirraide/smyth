#ifndef SMYTH_UI_MAINWINDOW_HH
#define SMYTH_UI_MAINWINDOW_HH

#include "SmythPlainTextEdit.hh"

#include <QMainWindow>
#include <QStringListModel>
#include <UI/App.hh>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace smyth::ui {
class SmythRichTextEdit;
class MainWindow final : public QMainWindow {
    Q_OBJECT

    std::unique_ptr<Ui::MainWindow> ui;

    /// Model for character select table.
    std::unique_ptr<QStringListModel> character_select_model;

    /// App is the main controller of the program and needs to call protected
    /// functions sometimes.
    friend App;

    QMenu* notes_tab_context_menu;

public:
    LIBBASE_IMMOVABLE(MainWindow);
    MainWindow();
    ~MainWindow() noexcept;

    /// Handle a user pressing the close button.
    void closeEvent(QCloseEvent* event) override;

    /// Usually does nothing, but may be defined to do something if
    /// App needs to access the ui.
    [[maybe_unused]] void debug();

    /// Show the window to the user. This is called once during startup.
    void init();

    /// Text box used by the notes tab.
    auto notes_tab_text_box() -> SmythPlainTextEdit*;

    /// Initialise persistent objects.
    void persist();

    /// Reset the window to its default settings.
    void reset_window();

    /// Set the path to be shown in the window title.
    void set_window_path(QString path);

    void HandleErrors(Result<> r);

public slots:
    void apply_sound_changes();
    void char_map_update_selection(char32_t c);
    void new_project();
    void open_project();
    void open_settings();
    void preview_changes_after_eval();
    void prompt_quit();
    void save_project();

private:
    auto ApplySoundChanges() -> Result<>;
    auto EvaluateAndInterpolateJavaScript(QString& in_string) -> Result<>;
};
} // namespace smyth::ui
#endif // SMYTH_UI_MAINWINDOW_HH

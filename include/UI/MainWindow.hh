#ifndef SMYTH_UI_MAINWINDOW_HH
#define SMYTH_UI_MAINWINDOW_HH

#include <QMainWindow>
#include <QStringListModel>
#include <UI/Smyth.hh>
#include <UI/SmythPlainTextEdit.hh>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace smyth::ui {
class SmythRichTextEdit;
class MainWindow final : public QMainWindow {
    Q_OBJECT
    LIBBASE_IMMOVABLE(MainWindow);
    friend void ui::InitialiseSmyth();

    std::unique_ptr<Ui::MainWindow> ui;

    /// The main window instance. There is only one.
    static MainWindow* Instance;

    QMenu* notes_tab_context_menu;

    MainWindow();

public:
    ~MainWindow() noexcept override;

    /// Get the text box used by the notes tab.
    static auto GetNotesTabTextBox() -> SmythPlainTextEdit*;

    /// Prompt the user.
    static auto Prompt(
        const QString& title,
        const QString& message,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No
    ) -> QMessageBox::StandardButton;

    /// Reset the window to its default settings.
    static void Reset();

    /// Set the path to be shown in the window title. If the
    /// path is empty, it is set to 'Smyth' instead.
    static void SetWindowPath(QString path = "");

    /// Show an error to the user.
    static auto ShowError(
        const QString& error,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        const QString& title = "Error"
    ) -> QMessageBox::StandardButton;

    /// Handle a user pressing the close button.
    void closeEvent(QCloseEvent* event) override;

public slots:
    void apply_sound_changes();
    void char_map_update_selection(char32_t c);
    void new_project();
    void open_project();
    void open_settings();
    void preview_changes_after_eval();
    void prompt_quit();
    void save_project();
    void show_project_directory();

private:
    auto ApplySoundChanges() -> Result<>;
    auto EvaluateAndInterpolateJavaScript(QString& in_string) -> Result<>;
    void Init();
    void Persist();
};
} // namespace smyth::ui
#endif // SMYTH_UI_MAINWINDOW_HH

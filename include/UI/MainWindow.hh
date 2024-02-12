#ifndef SMYTH_UI_MAINWINDOW_HH
#define SMYTH_UI_MAINWINDOW_HH

#include <QMainWindow>
#include <QStringListModel>
#include <UI/App.hh>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


namespace smyth::ui {
class SettingsDialog;
class VFSTreeModel;

class MainWindow final : public QMainWindow {
    Q_OBJECT

    std::unique_ptr<Ui::MainWindow> ui;

    /// Model for character select table.
    std::unique_ptr<QStringListModel> character_select_model;

    /// Model for VFS tree.
    VFSTreeModel* const vfs_tree_model;

    /// Popup menu for VFS tree.
    QMenu* const vfs_context_menu;

    /// Current position of the vfs context menu.
    QPoint vfs_context_menu_pos;

    /// App is the main controller of the program and needs to call protected
    /// functions sometimes.
    friend App;

public:
    SMYTH_IMMOVABLE(MainWindow);
    MainWindow();
    ~MainWindow() noexcept;

    /// Handle a user pressing the close button.
    void closeEvent(QCloseEvent* event) override;

    /// Usually does nothing, but may be defined to do something if
    /// App needs to access the ui.
    [[maybe_unused]] void debug();

    /// Show the window to the user. This is called once during startup.
    void init();

    /// Get the current default monospace font.
    auto mono_font() const -> const QFont&;

    /// Initialise persistent objects.
    void persist();

    /// Get the current default serif font.
    auto serif_font() const -> const QFont&;

    /// Set the default monospace font.
    void set_mono_font(QFont f);

    /// Set the default serif font.
    void set_serif_font(QFont f);

    /// Set the path to be shown in the window title.
    void set_window_path(QString path);

public slots:
    void apply_sound_changes();
    void char_map_update_selection(char32_t c);
    void new_project();
    void open_project();
    void open_settings();
    void preview_changes_after_eval();
    void save_project();
    void show_vfs_context_menu(QPoint pos);
    void vfs_new_file();

private:
    auto ApplySoundChanges() -> Result<>;
    auto EvaluateAndInterpolateJavaScript(QString& in_string) -> Result<>;
    void HandleErrors(Result<> r);
};
} // namespace smyth::ui
#endif // SMYTH_UI_MAINWINDOW_HH

#ifndef SMYTH_UI_APP_HH
#define SMYTH_UI_APP_HH

#include <chrono>
#include <mutex>
#include <QCloseEvent>
#include <Smyth/Utils.hh>
#include <UI/Lexurgy.hh>
#include <UI/PersistObjects.hh>
#include <UI/UserSettings.hh>
#include <UI/Utils.hh>

namespace smyth::ui {
namespace chr = std::chrono;
class App;
} // namespace smyth::ui

/// FIXME: WHY is this a singleton? Move any functions into a namespace,
///     'Utils', or the main window, and move class members into their
///     constituent classes, and add a 'RunApplication' function.
class smyth::ui::App final {
    /// Hack to make sure 'the_app' is initialised before everything else.
    struct _init {
    } _init_;

    /// Lexurgy background process.
    std::unique_ptr<Lexurgy> lexurgy_ptr;

    /// The path to the project that is currently open.
    QString save_path;

    /// When we last saved the project, if ever.
    std::optional<chr::time_point<chr::system_clock>> last_save_time;

    /// Windows.
    std::unique_ptr<MainWindow> main;
    std::unique_ptr<SettingsDialog> settings;

    /// Global app.
    static App* the_app;

public:
    /// Used for global settings.
    PersistentStore global_store;

    /// Per-user settings.
    UserSetting<QFont> mono_font{"mono.font", QFont{"monospace"}};
    UserSetting<QFont> serif_font{"serif.font", QFont{"serif"}};
    UserSetting<> last_open_project{"last_open_project", ""};

#ifdef SMYTH_DEBUG
    UserSetting<bool> dump_json_requests{"__debug__/dump_json_requests", false};
#endif

    SMYTH_IMMOVABLE(App);
    App();
    ~App() noexcept;

    /// Apply sound changes to the input string.
    auto apply_sound_changes(
        QString inputs,
        QString sound_changes,
        QString stop_before
    ) -> Result<QString>;

    /// Open a new project.
    void new_project();

    /// Open a project from a file.
    auto open() -> Result<>;

    /// Ask the user if we should close the current project.
    ///
    /// \return Whether we should close the project.
    bool prompt_close_project();

    /// Save project to a file.
    auto save() -> Result<>;

    /// Get the settings dialog.
    auto settings_dialog() -> SettingsDialog* { return settings.get(); }

    /// Create a table in a store.
    static auto CreateStore(
        std::string name,
        PersistentStore& parent = The().global_store
    ) -> PersistentStore&;

    /// Get the main window.
    static auto MainWindow() -> MainWindow* { return the_app->main.get(); }

    /// Show an error to the user.
    static void ShowError(const QString& error, const QString& title = "Error");

    /// Get the global app.
    static auto The() -> App& { return *the_app; }

private:
    /// Create or get the current lexurgy instance.
    auto GetLexurgy() -> Result<Lexurgy&>;

    /// Load the last project we had open.
    void LoadLastOpenProject();

    /// Open a project.
    auto OpenProject(QString path) -> Result<>;

    /// Save project to a file.
    ///
    /// This only handles the actual saving; everything else must
    /// be done before that.
    auto SaveImpl() -> Result<>;
};

#endif // SMYTH_UI_APP_HH

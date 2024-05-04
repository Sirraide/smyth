#ifndef SMYTH_UI_APP_HH
#define SMYTH_UI_APP_HH

#include <chrono>
#include <mutex>
#include <QCloseEvent>
#include <Smyth/Database.hh>
#include <Smyth/Result.hh>
#include <Smyth/Utils.hh>
#include <UI/Lexurgy.hh>
#include <UI/PersistObjects.hh>
#include <UI/Utils.hh>

namespace smyth::ui {
namespace chr = std::chrono;
class App;
} // namespace smyth::ui

class smyth::ui::App final {
    /// Lexurgy background process.
    std::unique_ptr<Lexurgy> lexurgy = std::make_unique<Lexurgy>();

    /// In-memory database; this holds a copy of the database stored on
    /// disk that is the actual project file and is used for saving and
    /// checking if we need to save.
    ///
    /// TODO: Make this the actual DB and add an auto-save feature.
    DBRef db = Database::CreateInMemory();

    /// The path to the project that is currently open.
    QString save_path;

    /// When we last saved the project, if ever.
    std::optional<chr::time_point<chr::system_clock>> last_save_time;

    /// Windows.
    std::unique_ptr<MainWindow> main;
    std::unique_ptr<SettingsDialog> settings;

    /// Used for operations like saving and closing that may contend with one
    /// another or cause data corruption if executed at the same time, though
    /// not for everything.
    ///
    /// Save on quit is part of why this is recursive.
    std::recursive_mutex global_lock;

    /// Global app.
    static App* the_app;

public:
    /// Store that holds persistent data.
    PersistentStore store{SMYTH_MAIN_STORE_KEY};

    SMYTH_IMMOVABLE(App);
    App(ErrorMessageHandler handler);
    ~App() noexcept;

    /// Persist a QString property in the store.
    template <auto Get, auto Set, typename Object>
    auto persist(
        std::string key,
        Object* obj,
        usz priority = smyth::detail::DefaultPriority
    ) -> smyth::detail::PersistentBase* {
        using namespace smyth::detail;
        using namespace detail;
        using Property = PersistProperty<ExtractType<decltype(Get)>, Object, Get, Set>;
        std::unique_ptr<PersistentBase> e{new Property(obj)};
        auto ptr = e.get();
        store.register_entry(std::move(key), {std::move(e), priority});
        return ptr;
    }

    /// Apply sound changes to the input string.
    auto apply_sound_changes(
        QString inputs,
        QString sound_changes,
        QString stop_before
    ) -> Result<QString>;

    /// Load the last project we had open.
    auto load_last_open_project() -> Result<>;

    /// Open a new project.
    void new_project();

    /// Open a project from a file.
    auto open() -> Result<>;

    /// Quit the program.
    void quit(QCloseEvent* e);

    /// Save project to a file.
    auto save() -> Result<>;

    /// Get the settings dialog.
    auto settings_dialog() -> SettingsDialog* { return settings.get(); }

    /// Get the main window.
    static auto MainWindow() -> MainWindow* { return the_app->main.get(); }

    /// Get the global app.
    static auto The() -> App& { return *the_app; }

private:
    /// Remember the last project we had open.
    void NoteLastOpenProject();

    /// Open a project.
    auto OpenProject(QString path) -> Result<>;

    /// Ask the user if we should close the current project.
    bool PromptCloseProject();

    /// Save project to a file.
    ///
    /// This only handles the actual saving; everything else must
    /// be done before that.
    auto SaveImpl() -> Result<>;
};

#endif // SMYTH_UI_APP_HH

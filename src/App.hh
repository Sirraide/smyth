#ifndef SMYTH_APP_HH
#define SMYTH_APP_HH

#include <chrono>
#include <Database.hh>
#include <Lexurgy.hh>
#include <PersistObjects.hh>
#include <QCloseEvent>
#include <QObject>
#include <QUrl>
#include <Result.hh>
#include <Utils.hh>

#define SMYTH_MAIN_STORE_KEY "store"

namespace smyth {
namespace chr = std::chrono;

namespace detail {
template <typename>
struct ExtractTypeImpl;

template <typename Type, typename Object>
struct ExtractTypeImpl<Type(Object::*)> {
    using type = Type;
};

template <typename Type, typename Object>
struct ExtractTypeImpl<Type (Object::*)() const> {
    using type = Type;
};

template <typename Type>
using ExtractType = typename ExtractTypeImpl<Type>::type;

} // namespace detail

class MainWindow;
class SettingsDialog;

class App : public QObject {
    Q_OBJECT

    Lexurgy lexurgy;
    DBRef db = Database::CreateInMemory();
    QString save_path;
    std::unique_ptr<MainWindow> main;
    std::unique_ptr<SettingsDialog> settings;
    std::optional<chr::time_point<chr::system_clock>> last_save_time;

    /// Used for operations like saving and closing that may content with one
    /// another or cause data corruption if executed at the same time, though
    /// not for everything.
    ///
    /// Save on quit is part of why this is recursive.
    std::recursive_mutex global_lock;

public:
    PersistentStore store{SMYTH_MAIN_STORE_KEY};

    SMYTH_IMMOVABLE(App);
    App();
    ~App() noexcept;

    /// Persist a QString property in the store.
    template <auto Get, auto Set, typename Object>
    void persist(std::string key, Object* obj) {
        using namespace detail;
        std::unique_ptr<PersistentBase> e{new PersistProperty<ExtractType<decltype(Get)>, Object, Get, Set>(obj)};
        store.register_entry(std::move(key), std::move(e));
    }

    /// Apply sound changes to the input string.
    auto apply_sound_changes(
        QString inputs,
        QString sound_changes,
        QString stop_before
    ) -> Result<QString>;

    /// Load the last project we had open.
    auto load_last_open_project() -> Result<>;

    /// Get the main window.
    auto main_window() -> MainWindow* { return main.get(); }

    /// Open a project from a file.
    auto open() -> Result<>;

    /// Quit the program.
    void quit(QCloseEvent* e);

    /// Save project to a file.
    auto save() -> Result<>;

    /// Get the settings dialog.
    auto settings_dialog() -> SettingsDialog* { return settings.get(); }

private:
    /// Remember the last project we had open.
    void NoteLastOpenProject();

    /// Open a project.
    auto OpenProject(QString path) -> Result<>;

    /// Save project to a file.
    ///
    /// This only handles the actual saving; everything else must
    /// be done before that.
    auto SaveImpl() -> Result<>;
};

} // namespace smyth

template <>
struct fmt::formatter<QString> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const QString& s, FormatContext& ctx) {
        return fmt::formatter<std::string>::format(s.toStdString(), ctx);
    }
};

#endif // SMYTH_APP_HH

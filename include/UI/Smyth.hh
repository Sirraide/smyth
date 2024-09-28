#ifndef SMYTH_UI_APP_HH
#define SMYTH_UI_APP_HH

#include <chrono>
#include <mutex>
#include <QCloseEvent>
#include <UI/PersistObjects.hh>
#include <UI/UserSettings.hh>

#define SMYTH_CURRENT_CONFIG_FILE_VERSION 1

namespace smyth::ui {
namespace chr = std::chrono;
/// Show errors to the user.
void HandleErrors(Result<> r);

/// Initialise the project.
void InitialiseSmyth();

/// All the state that pertains to a project.
class Project {
    friend void ui::InitialiseSmyth();

    /// The path to the project that is currently open.
    QString SavePath;

    /// When we last saved the project, if ever.
    chr::time_point<chr::system_clock> LastSaveTime;

    /// Create a new project.
    explicit Project(QString save_path);

public:
    Project() = default;

    /// Create a new project.
    static void New();

    /// Open a project.
    static void Open(QString path);

    /// Open the project directory.
    static void OpenDirInNativeShell();

    /// Prompt whether we should close this project.
    static bool PromptClose();

    /// Save the project.
    static void Save();

private:
    static auto OpenImpl(QString path, bool opening_last_open_project) -> Result<>;
    static void OpenLast();
    static auto SaveImpl() -> Result<>;
};
} // namespace smyth::ui

#endif // SMYTH_UI_APP_HH

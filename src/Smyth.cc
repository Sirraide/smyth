#include <base/FS.hh>
#include <filesystem>
#include <QDesktopServices>
#include <QFileDialog>
#include <UI/Lexurgy.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/Smyth.hh>
#include <UI/SmythCharacterMap.hh>
#include <UI/SmythPlainTextEdit.hh>
using namespace smyth;
using namespace smyth::ui;

// ====================================================================
//  Initialisation
// ====================================================================
void ui::InitialiseSmyth() {
    {
        static bool initialised = false;
        Assert(not initialised, "Already initialised");
        initialised = true;
    }

    // Create all windows that live for ever.
    MainWindow::Instance = new MainWindow;
    SettingsDialog::Instance = new SettingsDialog(MainWindow::Instance);

    // Initialise them separately. This is to ensure all windows exist
    // in case they need to refer to each other during initialisation.
    MainWindow::Instance->Init();
    SettingsDialog::Instance->Init();

    // Load user settings.
    detail::user_settings::Init();

    // Reopen the last project we had open, if any.
    Project::OpenLast();
}

// ====================================================================
//  Global Helpers.
// ====================================================================
void ui::HandleErrors(Result<> r) {
    if (not r) MainWindow::ShowError(QString::fromStdString(r.error()));
}

// ====================================================================
//  Project.
// ====================================================================
/// The current project. This exists so we can guarantee
/// that no project-specific state is leaked between projects.
Project CurrentProject;

Project::Project(QString path)
    : SavePath(std::move(path)),
      LastSaveTime(chr::system_clock::now()) {}

void Project::New() {
    if (not PromptClose()) return;
    CurrentProject = {};
    lexurgy::Close();
    PersistentStore::Global.reset_all();
    SettingsDialog::Reset();
    MainWindow::Reset();
}

bool Project::PromptClose() {
    enum struct State {
        UnsavedChanges,
        Cancel,
        Close,
        Save,
    } state = State::UnsavedChanges;
    using enum State;

    // Something went wrong. Ask the user if we should try again.
    auto RetryOnError = [&](std::string&& err) {
        // Include the 'Save' button only if checking for unsaved changes errored.
        auto buttons = QMessageBox::Yes | QMessageBox::Retry | QMessageBox::Cancel;
        auto str = std::format(
            "{} caused an error: {}.\n\nClose anyway?",
            state == Save ? "Saving" : "Checking for unsaved changes",
            err
        );

        // Show the error to the user.
        switch (MainWindow::ShowError(QString::fromStdString(str), buttons)) {
            case QMessageBox::Retry: break;
            case QMessageBox::Yes: state = Close; break;
            case QMessageBox::Save: state = Save; break;
            default: state = Cancel;
        }
    };

    // No project.
    if (CurrentProject.SavePath.isEmpty()) return true;

    // Run the state machine.
    for (;;) {
        switch (state) {
            // Prompt if the user wants to save.
            case UnsavedChanges: {
                std::string text;

                // We show the user when the project was last saved so they can more
                // accurately gauge how much work they’d lose if they don’t save. We
                // do *not* update the time in the dialog so that e.g. if it shows
                // '5 seconds', it will still show '5 seconds' if they leave the dialog
                // open for a while because this is about how much work they’d lose, not
                // about how long the dialog has been open (thanks also to Andreas Kling
                // for that notion, if I remember correctly).
                auto now = chr::system_clock::now();
                auto mins = chr::duration_cast<chr::minutes>(now - CurrentProject.LastSaveTime);
                auto secs = chr::duration_cast<chr::seconds>(now - CurrentProject.LastSaveTime);
                text = std::format(
                    "Last save was {} {} ago. Save project before exiting?",
                    mins.count() == 0 ? secs.count() : mins.count(),
                    mins.count() == 0 ? "seconds" : "minutes"
                );

                // Check if they want to save first.
                auto res = MainWindow::Prompt(
                    "Unsaved changes",
                    QString::fromStdString(text),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
                );

                switch (res) {
                    default: state = Cancel; break;
                    case QMessageBox::No: state = Close; break;
                    case QMessageBox::Yes: state = Save; break;
                }
            } break;

            // Don’t quit.
            case Cancel: return false;

            // Close without (or after) saving.
            case Close: return true;

            // Save and then quit.
            case Save: {
                auto res = SaveImpl();
                if (not res) RetryOnError(std::move(res).error());
                else state = Close;
            } break;
        }
    }
}

void Project::Open(QString path) {
    HandleErrors(OpenImpl(std::move(path), false));
}

void Project::OpenDirInNativeShell() {
    if (CurrentProject.SavePath.isEmpty()) return;

    // Remove the filename from the path.
    auto path = fs::path(CurrentProject.SavePath.toStdString()).parent_path();
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path.string())));
}

auto Project::OpenImpl(QString path, bool opening_last_open_project) -> Result<> {
    // Open the file.
    QFile f{path};
    if (not f.exists()) return Error("File '{}' does not exist", path.toStdString());
    if (not f.open(QIODevice::ReadOnly | QIODevice::Text)) return Error(
        "Could not open file '{}'",
        path.toStdString()
    );

    // Parse it.
    auto tt = Try(json_utils::Parse(f.readAll().toStdString()));

    // Check that the version number is correct.
    auto v = tt.contains("version") and tt["version"].is_number_unsigned() ? tt["version"].get<u64>() : 0;
    if (v != SMYTH_CURRENT_CONFIG_FILE_VERSION) return Error(
        "Sorry, your project file was created with a different version of Smyth "
        "({} vs expected {}); we currently don’t support automatic migration, so "
        "please migrate it manually or ask for help.",
        v,
        SMYTH_CURRENT_CONFIG_FILE_VERSION
    );

    // Reload all settings. If there is an error, abort and load the previous project.
    PersistentStore::Global.reset_all();
    auto res = PersistentStore::Global.reload_all(tt);
    if (not res) {
        // Reset settings again.
        PersistentStore::Global.reset_all();

        // Prevent infinite loops in case the last open project is broken.
        if (opening_last_open_project) {
            New();

            // Opening the last open project failed, so discard it from the
            // settings as there is clearly a problem with it.
            settings::LastOpenProject.set("");
            return res;
        }

        // Try to open the last project we had open.
        OpenLast();
        return res;
    }

    // Update save path and remember it.
    CurrentProject = Project(path);
    settings::LastOpenProject.set(path);
    return {};
}

void Project::OpenLast() {
    auto& path = *settings::LastOpenProject;
    if (path.isEmpty()) return;
    if (not File::Exists(path.toStdString())) return;
    HandleErrors(OpenImpl(std::move(path), true));
}

void Project::Save() {
    // If we already have a save path, use that.
    auto& save_path = CurrentProject.SavePath;
    if (not save_path.isEmpty()) return HandleErrors(SaveImpl());

    // Determine save path.
    save_path = QFileDialog::getSaveFileName(
        nullptr,
        "Save Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    // Append ‘.smyth’ by default.
    if (not save_path.contains(".")) save_path += ".smyth";

    // Don’t do anything if the user pressed Cancel, in which
    // case the save file name will be empty.
    if (not save_path.isEmpty()) return HandleErrors(SaveImpl());
}

auto Project::SaveImpl() -> Result<> {
    // Dew it.
    auto j = Try(PersistentStore::Global.save_all());
    Assert(not j.contains("version"), "Top-level 'version' key already set?");
    j["version"] = SMYTH_CURRENT_CONFIG_FILE_VERSION;
    Try(File::Write(CurrentProject.SavePath.toStdString(), j.dump(4)));

    // Update last save time.
    CurrentProject.LastSaveTime = std::chrono::system_clock::now();

    // Set this in case we’re saving a new project.
    settings::LastOpenProject.set(CurrentProject.SavePath);
    return {};
}

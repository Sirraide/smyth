#include <App.hh>
#include <filesystem>
#include <MainWindow.hh>
#include <mutex>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <SettingsDialog.hh>

namespace fs = std::filesystem;

#define SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY "last_open_project"

/// ====================================================================
///  App
/// ====================================================================
smyth::App::~App() noexcept = default;
smyth::App::App() {
    /// Do not use a member init list for these as it has to be
    /// constructed *after* everything else.
    main = std::make_unique<MainWindow>(*this);
    settings = std::make_unique<SettingsDialog>(*this);
}

auto smyth::App::apply_sound_changes(
    QString inputs,
    QString sound_changes,
    QString stop_before
) -> Result<QString> {
    return lexurgy(inputs, std::move(sound_changes), stop_before);
}

auto smyth::App::load_last_open_project() -> Result<> {
    /// Check if we have a last open project.
    QSettings s{QSettings::UserScope};
    auto path = s.value(SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY).toString();
    if (path.isEmpty()) return {};
    if (not fs::exists(path.toStdString())) return {};
    return OpenProject(std::move(path));
}

auto smyth::App::open() -> Result<> {
    auto path = QFileDialog::getOpenFileName(
        nullptr,
        "Open Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    if (path.isEmpty()) return {};
    return OpenProject(std::move(path));
}

void smyth::App::quit(QCloseEvent* e) {
    enum struct State {
        Start,
        UnsavedChanges,
        Cancel,
        Quit,
        Save,
    } state = State::Start;
    using enum State;

    /// Something went wrong. Ask the user if we should try again.
    auto RetryOnError = [&](Err err) {
        /// Include the 'Save' button only if checking for unsaved changes errored.
        auto buttons = QMessageBox::Yes | QMessageBox::Retry | QMessageBox::Cancel;
        if (state == Start) buttons |= QMessageBox::Save;
        auto str = fmt::format(
            "{} caused an error: {}.\n\nExit anyway?",
            state == Save ? "Saving" : "Checking for unsaved changes",
            err
        );

        /// Show the error to the user.
        auto retry = QMessageBox::critical(
            main_window(),
            "Error",
            QString::fromStdString(str),
            buttons
        );

        switch (retry) {
            case QMessageBox::Retry: break;
            case QMessageBox::Yes: state = Quit; break;
            case QMessageBox::Save: state = Save; break;
            default: state = Cancel;
        }
    };

    std::unique_lock _{global_lock};
    for (;;) {
        switch (state) {
            /// Saving is necessary if any persistent property has changed. Note
            /// that this only updates the in-memory DB, not the on-disk one.
            case Start: {
                auto modified = store.modified(db);
                if (modified.is_err()) {
                    RetryOnError(std::move(modified.err()));
                    break;
                }

                state = modified.value() ? State::UnsavedChanges : State::Quit;
            } break;

            /// Prompt if the user wants to save.
            case UnsavedChanges: {
                std::string text;

                /// We show the user when the project was last saved so they can more
                /// accurately gauge how much work they’d lose if they don’t save. We
                /// do *not* update the time in the dialog so that e.g. if it shows
                /// '5 seconds', it will still show '5 seconds' if they leave the dialog
                /// open for a while because this is about how much work they’d lose, not
                /// about how long the dialog has been open (thanks also to Andreas Kling
                /// for that notion, if I remember correctly).
                if (last_save_time.has_value()) {
                    auto now = chr::system_clock::now();
                    auto mins = chr::duration_cast<chr::minutes>(now - *last_save_time);
                    auto secs = chr::duration_cast<chr::seconds>(now - *last_save_time);
                    text = fmt::format(
                        "Last save was {} {} ago. Save project before exiting?",
                        mins.count() == 0 ? secs.count() : mins.count(),
                        mins.count() == 0 ? "seconds" : "minutes"
                    );
                } else {
                    text = "Save project before exiting?";
                }

                /// Check if they want to save first.
                auto res = QMessageBox::question(
                    main_window(),
                    "Unsaved changes",
                    QString::fromStdString(text),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
                );

                switch (res) {
                    default: state = Cancel; break;
                    case QMessageBox::No: state = Quit; break;
                    case QMessageBox::Yes: state = Save; break;
                }
            } break;

            /// Don’t quit.
            case Cancel: return e->ignore();

            /// Quit without saving.
            case Quit: return main_window()->QMainWindow::closeEvent(e);

            /// Save and then quit.
            case Save: {
                auto res = SaveImpl();
                if (res.is_err()) RetryOnError(std::move(res.err()));
                else state = Quit;
            }
        }
    }
}

auto smyth::App::save() -> Result<> {
    if (not save_path.isEmpty()) return SaveImpl();

    /// Determine save path.
    save_path = QFileDialog::getSaveFileName(
        nullptr,
        "Save Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    /// Append ".smyth" by default.
    if (not save_path.contains(".")) save_path += ".smyth";

    /// Don’t do anything if the user pressed Cancel.
    if (save_path.isEmpty()) return SaveImpl();
    return {};
}

void smyth::App::NoteLastOpenProject() {
    QSettings s{QSettings::UserScope};
    s.setValue(SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY, save_path);
}

auto smyth::App::OpenProject(QString path) -> Result<> {
    /// Load it.
    db = Try(Database::Load(path.toStdString()));
    Try(store.reload_all(db));

    /// Update save path and remember it.
    save_path = std::move(path);
    NoteLastOpenProject();
    return {};
}

auto smyth::App::SaveImpl() -> Result<> {
    /// Don’t save twice.
    std::unique_lock _{global_lock};
    Try(store.save_all(db));
    Try(db->backup(save_path.toStdString()));

    /// Update last save time.
    last_save_time = std::chrono::system_clock::now();
    NoteLastOpenProject();
    return {};
}

#include <filesystem>
#include <mutex>
#include <QCoreApplication>
#include <QFileDialog>
#include <QLayout>
#include <QMessageBox>
#include <QSettings>
#include <UI/App.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>

namespace fs = std::filesystem;

#define SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY "last_open_project"

/// ====================================================================
///  App
/// ====================================================================
smyth::ui::App* smyth::ui::App::the_app = nullptr;
smyth::ui::App::~App() noexcept = default;
smyth::ui::App::App(ErrorMessageHandler handler) {
    the_app = this;

    /// Register error handler.
    smyth::RegisterMessageHandler(handler);

    /// Do not use a member init list for these as they have to be
    /// constructed *after* everything else. Also, only persist objects
    /// *after* showing the window so saving the default settings works
    /// properly.
    main = std::make_unique<class MainWindow>();
    settings = std::make_unique<SettingsDialog>();
    main->init();
}

void smyth::ui::App::NoteLastOpenProject() {
    QSettings s{QSettings::UserScope};
    s.setValue(SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY, save_path);
    MainWindow()->set_window_path(save_path);
}

auto smyth::ui::App::OpenProject(QString path) -> Result<> {
    /// Load it.
    db = Try(Database::Load(path.toStdString()));
    Try(store.reload_all(db));
    settings->reset_dialog();

    /// Update save path and remember it.
    save_path = std::move(path);
    NoteLastOpenProject();
    return {};
}

bool smyth::ui::App::PromptCloseProject() {
    enum struct State {
        Start,
        UnsavedChanges,
        Cancel,
        Close,
        Save,
    } state = State::Start;
    using enum State;

    /// Something went wrong. Ask the user if we should try again.
    auto RetryOnError = [&](Err err) {
        /// Include the 'Save' button only if checking for unsaved changes errored.
        auto buttons = QMessageBox::Yes | QMessageBox::Retry | QMessageBox::Cancel;
        if (state == Start) buttons |= QMessageBox::Save;
        auto str = fmt::format(
            "{} caused an error: {}.\n\nClose anyway?",
            state == Save ? "Saving" : "Checking for unsaved changes",
            err
        );

        /// Show the error to the user.
        auto retry = QMessageBox::critical(
            MainWindow(),
            "Error",
            QString::fromStdString(str),
            buttons
        );

        switch (retry) {
            case QMessageBox::Retry: break;
            case QMessageBox::Yes: state = Close; break;
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

                state = modified.value() ? State::UnsavedChanges : State::Close;
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
                    MainWindow(),
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

            /// Don’t quit.
            case Cancel: return false;

            /// Close without (or after) saving.
            case Close: return true;

            /// Save and then quit.
            case Save: {
                auto res = SaveImpl();
                if (res.is_err()) RetryOnError(std::move(res.err()));
                else state = Close;
            } break;
        }
    }
}

auto smyth::ui::App::SaveImpl() -> Result<> {
    /// Don’t save twice.
    std::unique_lock _{global_lock};
    Try(store.save_all(db));
    Try(db->backup(save_path.toStdString()));

    /// Update last save time.
    last_save_time = std::chrono::system_clock::now();
    NoteLastOpenProject();
    return {};
}

auto smyth::ui::App::apply_sound_changes(
    QString inputs,
    QString sound_changes,
    QString stop_before
) -> Result<QString> {
    return lexurgy->apply(inputs, std::move(sound_changes), stop_before);
}

auto smyth::ui::App::load_last_open_project() -> Result<> {
    /// Check if we have a last open project.
    QSettings s{QSettings::UserScope};
    auto path = s.value(SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY).toString();
    if (path.isEmpty()) return {};
    if (not fs::exists(path.toStdString())) return {};
    return OpenProject(std::move(path));
}

void smyth::ui::App::new_project() {
    if (not PromptCloseProject()) return;
    db = Database::CreateInMemory();
    save_path = "";
    last_save_time = std::nullopt;
    lexurgy = std::make_unique<Lexurgy>();
    store.reset_all();
    settings->reset_dialog();
    MainWindow()->set_window_path("");
}

auto smyth::ui::App::open() -> Result<> {
    auto path = QFileDialog::getOpenFileName(
        nullptr,
        "Open Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    if (path.isEmpty()) return {};
    return OpenProject(std::move(path));
}

void smyth::ui::App::quit(QCloseEvent* e) {
    if (not PromptCloseProject()) return e->ignore();
    MainWindow()->QMainWindow::closeEvent(e);
}

auto smyth::ui::App::save() -> Result<> {
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
    if (not save_path.isEmpty()) return SaveImpl();
    return {};
}

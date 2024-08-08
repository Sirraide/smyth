#include <base/Defer.hh>
#include <base/FS.hh>
#include <filesystem>
#include <QFileDialog>
#include <QMessageBox>
#include <UI/App.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/SmythCharacterMap.hh>
#include <UI/SmythPlainTextEdit.hh>

import smyth.utils;
import smyth.lexurgy;

using namespace smyth::ui;

/// ====================================================================
///  App
/// ====================================================================
App* App::the_app = nullptr;
App::~App() noexcept = default;
App::App() : _init_{[&] { the_app = this; return _init{}; }()} {
    // Do not use a member init list for these as they have to be
    // constructed *after* everything else. Also, only persist objects
    // *after* showing the window so saving the default settings works
    // properly.
    main = std::make_unique<class MainWindow>();
    settings = std::make_unique<SettingsDialog>();
    main->init();
    settings->init();
    detail::user_settings::Init();
    MainWindow()->setWindowTitle("Smyth");
    LoadLastOpenProject();
}

auto App::CreateStore(std::string name, PersistentStore& parent) -> PersistentStore& {
    auto store = new PersistentStore;
    parent.register_entry(
        std::move(name),
        {std::unique_ptr<smyth::detail::PersistentBase>{store}, smyth::detail::DefaultPriority}
    );
    return *store;
}

void App::LoadLastOpenProject() {
    // Check if we have a last open project.
    tempset opening_last_open_project = true;
    auto& path = *last_open_project;
    std::print("LAST OPEN PROJECT: {}\n", path);
    if (path.isEmpty()) return;
    if (not File::Exists(path.toStdString())) return;
    MainWindow()->HandleErrors(OpenProject(std::move(path)));
}

auto App::OpenProject(QString path) -> Result<> {
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
    global_store.reset_all();
    auto res = global_store.reload_all(tt);
    if (not res) {
        // Reset settings again.
        global_store.reset_all();

        // Prevent infinite loops in case the last open project is broken.
        if (opening_last_open_project) {
            new_project();
            last_open_project.set("");
            return res;
        }

        // Try to open the last project.
        LoadLastOpenProject();
        return res;
    }

    // Update save path and remember it.
    settings->reset_dialog();
    save_path = std::move(path);
    last_open_project.set(save_path);
    return {};
}

auto App::SaveImpl() -> Result<> {
    // Dew it.
    auto j = Try(global_store.save_all());
    Assert(not j.contains("version"), "Top-level 'version' key already set?");
    j["version"] = SMYTH_CURRENT_CONFIG_FILE_VERSION;
    Try(File::Write(save_path.toStdString(), j.dump(4)));

    // Update last save time.
    last_save_time = std::chrono::system_clock::now();
    last_open_project.set(save_path);
    return {};
}

void App::ShowError(const QString& error, const QString& title) {
    QMessageBox::critical(MainWindow(), title, error);
}

auto App::apply_sound_changes(
    QString inputs,
    QString sound_changes,
    QString stop_before
) -> Result<QString> {
    return Try(Lexurgy::Apply(inputs, std::move(sound_changes), stop_before));
}

void App::new_project() {
    if (not prompt_close_project()) return;
    save_path = "";
    last_save_time = std::nullopt;
    Lexurgy::Reset();
    global_store.reset_all();
    settings->reset_dialog();
    MainWindow()->reset_window();
}

auto App::open() -> Result<> {
    auto path = QFileDialog::getOpenFileName(
        nullptr,
        "Open Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    if (path.isEmpty()) return {};
    return OpenProject(std::move(path));
}

bool App::prompt_close_project() {
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
                if (last_save_time.has_value()) {
                    auto now = chr::system_clock::now();
                    auto mins = chr::duration_cast<chr::minutes>(now - *last_save_time);
                    auto secs = chr::duration_cast<chr::seconds>(now - *last_save_time);
                    text = std::format(
                        "Last save was {} {} ago. Save project before exiting?",
                        mins.count() == 0 ? secs.count() : mins.count(),
                        mins.count() == 0 ? "seconds" : "minutes"
                    );
                } else {
                    text = "Save project before exiting?";
                }

                // Check if they want to save first.
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

auto App::save() -> Result<> {
    if (not save_path.isEmpty()) return SaveImpl();

    // Determine save path.
    save_path = QFileDialog::getSaveFileName(
        nullptr,
        "Save Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    // Append ".smyth" by default.
    if (not save_path.contains(".")) save_path += ".smyth";

    // Don’t do anything if the user pressed Cancel.
    if (not save_path.isEmpty()) return SaveImpl();
    return {};
}

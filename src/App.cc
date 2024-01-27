#include <App.hh>
#include <filesystem>
#include <MainWindow.hh>
#include <mutex>
#include <QCoreApplication>
#include <QFileDialog>
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
) -> QString {
    auto res = lexurgy(inputs, sound_changes, stop_before);
    if (res.is_err()) throw Exception("{}", res.err().message);
    return res.value();
}

void smyth::App::load_last_open_project() {
    QSettings settings{QSettings::UserScope};
    auto path = settings.value(SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY).toString();
    if (path.isEmpty()) return;
    if (not fs::exists(path.toStdString())) return;
    auto res = Database::Load(path.toStdString());
    if (res.is_err()) {
        Error("Failed to load last open project: {}", res.err());
        return;
    }

    try {
        db = std::move(res.value());
        store.reload_all(db);
        save_path = std::move(path);
        NoteLastOpenProject();
    } catch (const std::exception& e) {
        Error("Failed to load last open project: {}", e.what());
    }
}

void smyth::App::open() try {
    auto path = QFileDialog::getOpenFileName(
        nullptr,
        "Open Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    if (path.isEmpty()) return;
    auto res = Database::Load(path.toStdString());
    if (res.is_err()) throw Exception("{}", res.err());

    db = std::move(res.value());
    store.reload_all(db);
    save_path = std::move(path);
    NoteLastOpenProject();
} catch (const Exception& e) {
    Error("Failed to open project: {}", e.what());
}

void smyth::App::save() {
    if (not save_path.isEmpty()) {
        SaveImpl();
        return;
    }

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
    if (save_path.isEmpty()) return;
    SaveImpl();
}

void smyth::App::NoteLastOpenProject() {
    QSettings settings{QSettings::UserScope};
    settings.setValue(SMYTH_QSETTINGS_LAST_OPEN_PROJECT_KEY, save_path);
}

void smyth::App::SaveImpl() try {
    /// Don’t save twice.
    std::unique_lock _{save_lock};
    store.save_all(db);
    auto res = db->backup(save_path.toStdString());
    if (res.is_err()) {
        Error("Failed to save project: {}", res.err());
        return;
    }

    NoteLastOpenProject();
} catch (const Exception& e) {
    Error("Failed to save project: {}", e.what());
}

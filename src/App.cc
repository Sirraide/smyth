#include <App.hh>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <sqlite3.h>

/// ====================================================================
///  App
/// ====================================================================
smyth::App::App() {
    /// Create main table.
    auto err = db.exec(R"sql(
        CREATE TABLE IF NOT EXISTS store (
            key TEXT PRIMARY KEY,
            value BLOB
        );
    )sql");
    if (err.is_err()) App::ShowFatalError("Failed to initialise database: {}", err.err());
}

auto smyth::App::applySoundChanges(QString inputs, QString sound_changes) -> QString {
    auto res = lexurgy(inputs, sound_changes);
    if (res.is_err()) {
        ShowError(QString::fromStdString(res.err().message));
        return "ERROR";
    }

    return res.value();
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

void smyth::App::saveAs() {
    ShowError("TODO : saveAs()");
}

void smyth::App::SaveImpl() {
    auto res = db.backup(save_path.toStdString());
    if (res.is_err()) {
        ShowError("Failed to save project: {}", res.err());
        return;
    }
}

void smyth::App::ShowError(QString message) {
    QMessageBox box;
    box.setWindowTitle("Error");
    box.setIcon(QMessageBox::Critical);
    box.setText(message);
    box.setWindowFlags(box.windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint));
    box.exec();
}

void smyth::App::ShowFatalError(QString message) {
    ShowError(message);
    std::exit(-1);
}

/// ====================================================================
///  Database
/// ====================================================================
smyth::Database::Database() {
    auto res = sqlite3_open_v2(
        ":memory:",
        &handle,
        SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE |
            SQLITE_OPEN_MEMORY |
            SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (res != SQLITE_OK) App::ShowFatalError(
        "Failed to open database: {}",
        sqlite3_errmsg(handle)
    );
}

smyth::Database::~Database() noexcept {
    sqlite3_close_v2(handle);
}

auto smyth::Database::backup(std::string_view path) -> Res {
    sqlite3* target;
    auto res = sqlite3_open_v2(
        path.data(),
        &target,
        SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE |
            SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (res != SQLITE_OK) return fmt::format(
        "Failed to open database '{}': {}",
        path,
        sqlite3_errmsg(target)
    );

    sqlite3_backup* backup = sqlite3_backup_init(target, "main", handle, "main");
    if (backup == nullptr) return fmt::format(
        "Failed to create backup: {}",
        sqlite3_errmsg(target)
    );

    res = sqlite3_backup_step(backup, -1);
    if (res != SQLITE_DONE) return fmt::format(
        "Failed to backup database: {}",
        sqlite3_errmsg(target)
    );

    sqlite3_backup_finish(backup);
    sqlite3_close_v2(target);
    return {};
}

auto smyth::Database::exec(std::string_view query) -> Res {
    char* err = nullptr;
    auto res = sqlite3_exec(handle, query.data(), nullptr, nullptr, &err);
    if (res != SQLITE_OK) return fmt::format("Failed to execute query {}\nMessage: {}", query, err);
    return {};
}

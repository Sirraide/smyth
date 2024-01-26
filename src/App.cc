#include <App.hh>
#include <QCoreApplication>
#include <QFileDialog>

/// ====================================================================
///  App
/// ====================================================================
auto smyth::App::applySoundChanges(QString inputs, QString sound_changes) -> QString {
    auto res = lexurgy(inputs, sound_changes);
    if (res.is_err()) {
        Error("{}", res.err().message);
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
    Error("TODO : saveAs()");
}

void smyth::App::SaveImpl() {
    auto res = db.backup(save_path.toStdString());
    if (res.is_err()) {
        Error("Failed to save project: {}", res.err());
        return;
    }
}

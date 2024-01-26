#include <MainWindow.hh>
#include <ui_MainWindow.h>

namespace smyth {
namespace {
void PersistPTE(App& app, std::string key, QPlainTextEdit* te) {
    app.persist<&QPlainTextEdit::toPlainText, &QPlainTextEdit::setPlainText>(key, te);
}
}
}

/// Needs destructor that isn’t visible in the header.
smyth::MainWindow::~MainWindow() noexcept = default;

smyth::MainWindow::MainWindow(App& app)
    : QMainWindow(nullptr), ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);

    /// Initialise persistent settings.
    PersistPTE(app, "main.input.text", ui->input);
    PersistPTE(app, "main.changes.text", ui->changes);
}

#include <MainWindow.hh>
#include <QSettings>
#include <QShortcut>
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
    : QMainWindow(nullptr), app(app), ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);

    /// Initialise shortcuts.
    auto save = new QShortcut(QKeySequence::Save, this);
    auto open = new QShortcut(QKeySequence::Open, this);
    connect(save, &QShortcut::activated, this, &MainWindow::save_project);
    connect(open, &QShortcut::activated, this, &MainWindow::open_project);

    /// Initialise persistent settings.
    PersistPTE(app, "main.input.text", ui->input);
    PersistPTE(app, "main.changes.text", ui->changes);
    app.persist<&QWidget::size, [](QWidget* w, QSize s) { w->resize(s); }>("main.window.size", this);

    /// Load last open project, if any.
    app.load_last_open_project();
}

void smyth::MainWindow::apply_sound_changes() {
    auto text = app.apply_sound_changes(ui->input->toPlainText(), ui->changes->toPlainText());
    ui->output->setPlainText(std::move(text));
}

void smyth::MainWindow::open_project() {
    app.open();
}

void smyth::MainWindow::save_project() {
    app.save();
}

#include <MainWindow.hh>
#include <QSettings>
#include <QShortcut>
#include <ui_MainWindow.h>

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
    ui->input->persist(app, "main.input");
    ui->changes->persist(app, "main.changes");
    ui->output->persist(app, "main.output");
    app.persist<&QWidget::size, [](QWidget* w, QSize s) { w->resize(s); }>("main.window.size", this);
    app.persist<&QSplitter::sizes, &QSplitter::setSizes>("main.sca.splitter.sizes", ui->sca_text_edits);

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

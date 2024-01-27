#include <MainWindow.hh>
#include <QSettings>
#include <QShortcut>
#include <ui_MainWindow.h>
#include <Unicode.hh>
#include <QJSEngine>

namespace smyth {
namespace {
void PersistCBox(App& app, std::string key, QComboBox* cbox) {
    app.persist<&QComboBox::currentIndex, &QComboBox::setCurrentIndex>(
        std::move(key),
        cbox
    );
}

void PersistChBox(App& app, std::string key, QCheckBox* cbox) {
    app.persist<&QCheckBox::checkState, &QCheckBox::setCheckState>(
        std::move(key),
        cbox
    );
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
    ui->input->persist(app, "main.input");
    ui->changes->persist(app, "main.changes");
    ui->output->persist(app, "main.output");
    app.persist<&QWidget::size, [](QWidget* w, QSize s) { w->resize(s); }>("main.window.size", this);
    app.persist<&QSplitter::sizes, &QSplitter::setSizes>("main.sca.splitter.sizes", ui->sca_text_edits);
    PersistCBox(app, "main.sca.cbox.input.norm.choice", ui->sca_cbox_input_norm);
    PersistCBox(app, "main.sca.cbox.changes.norm.choice", ui->sca_cbox_changes_norm);
    PersistCBox(app, "main.sca.cbox.output.norm.choice", ui->sca_cbox_output_norm);
    PersistChBox(app, "main.sca.chbox.details", ui->sca_chbox_details);
    PersistChBox(app, "main.sca.chbox.enable.js", ui->sca_chbox_enable_javascript);

    /// Hide the details panels if the checkbox is unchecked.
    if (not ui->sca_chbox_details->isChecked()) {
        ui->sca_frame_input_bottom->setVisible(false);
        ui->sca_frame_changes_bottom->setVisible(false);
        ui->sca_frame_output_bottom->setVisible(false);
    }

    /// Load last open project, if any.
    app.load_last_open_project();
}

void smyth::MainWindow::apply_sound_changes() try {
    auto Norm = [](QComboBox* cbox, QString plain) {
        const auto norm = [cbox] {
            switch (cbox->currentIndex()) {
                default: return NormalisationForm::None;
                case 1: return NormalisationForm::NFC;
                case 2: return NormalisationForm::NFD;
            }
        }();

        auto normed = Normalise(
            norm,
            icu::UnicodeString(reinterpret_cast<char16_t*>(plain.data()), i32(plain.size()))
        );

        return QString::fromUtf16(normed.getBuffer(), normed.length());
    };

    auto input = Norm(ui->sca_cbox_input_norm, ui->input->toPlainText());
    auto changes = Norm(ui->sca_cbox_changes_norm, ui->changes->toPlainText());

    /// If javascript is enabled, find all instances of `§{}§` and replace them with
    /// the result of evaluating the javascript expression inside the braces.
    if (ui->sca_chbox_enable_javascript->isChecked()) {
        QJSEngine js;
        qsizetype pos = 0;
        for (;;) {
            auto next = changes.indexOf("§{", pos);
            if (next == -1) break;
            auto end = changes.indexOf("}§", next);
            if (end == -1) break;
            auto expr = changes.mid(next + 2, end - next - 2);
            auto result = js.evaluate(expr);
            if (result.isError()) {
                Error("Exception in JS Evaluation: {}\nWhile evaluating:\n{}", result.toString(), expr);
                return;
            }

            auto str = result.toString();
            changes.replace(next, end - next + 2, str);
            pos = next + str.size();
        }
    }

    auto output = app.apply_sound_changes(std::move(input), std::move(changes));
    ui->output->setPlainText(Norm(ui->sca_cbox_output_norm, std::move(output)));
} catch (const Exception& e) {
    Error("{}", e.what());
}

void smyth::MainWindow::open_project() {
    app.open();
}

void smyth::MainWindow::save_project() {
    app.save();
}

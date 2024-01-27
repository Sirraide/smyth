#include <MainWindow.hh>
#include <QJSEngine>
#include <QSettings>
#include <QShortcut>
#include <ui_MainWindow.h>
#include <Unicode.hh>

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

/// Like PersistCBox, but the entries are generated dynamically.
void PersistDynCBox(App& app, std::string key, QComboBox* cbox) {
    app.persist<&QComboBox::currentText, [](QComboBox* cbox, QString str) {
        /// If the item does not exists yet, add it.
        if (cbox->findText(str) == -1) cbox->addItem(str);
        cbox->setCurrentText(str);
    }>(std::move(key), cbox);
}

} // namespace
} // namespace smyth

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
    PersistDynCBox(app, "main.sca.cbox.stop.before", ui->sca_cbox_stop_before);
    PersistChBox(app, "main.sca.chbox.details", ui->sca_chbox_details);
    PersistChBox(app, "main.sca.chbox.enable.js", ui->sca_chbox_enable_javascript);

    /// Hide the details panels if the checkbox is unchecked.
    if (not ui->sca_chbox_details->isChecked()) {
        ui->sca_frame_input_bottom->setVisible(false);
        ui->sca_frame_changes_bottom->setVisible(false);
        ui->sca_frame_output_bottom->setVisible(false);
        ui->frame_stop_before->setVisible(false);
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

    /// Remember the 'Stop Before' rule that is currently selected.
    auto stop_before = ui->sca_cbox_stop_before->currentText();

    /// Parse the sound changes to figure out what rules we have for the
    /// 'Stop Before' dropdown; rule names are lines that start with a name
    /// followed by a colon.
    std::vector<QString> rule_names;
    for (auto line : changes.split('\n')) {
        line = line.trimmed();
        if (
            line.isEmpty() or
            line.startsWith('#') or
            line.startsWith('\\') or
            line.contains("=>")
        ) continue;

        /// If the line contains a colon, and everything before the colon
        /// does not contain whitespace, then this is the name of a rule.
        auto colon = line.indexOf(':');
        if (colon == -1) continue;
        auto name = line.mid(0, colon).trimmed();
        if (name.contains(' ')) continue;
        rule_names.push_back(name);
    }

    /// Filter out special rules.
    std::erase_if(rule_names, [](const QString& str) {
        return str.startsWith("romanizer") or
               str == "deromanizer" or
               str == "Syllables";
    });

    /// Update the 'Stop Before' dropdown.
    ui->sca_cbox_stop_before->clear();
    ui->sca_cbox_stop_before->addItem("");
    for (const auto& name : rule_names) ui->sca_cbox_stop_before->addItem(name);

    /// If the rule that was selected before still exists, select it again; if
    /// not, do not stop before any rule.
    if (rgs::contains(rule_names, stop_before)) ui->sca_cbox_stop_before->setCurrentText(stop_before);
    else stop_before = "";

    /// Dew it.
    auto output = app.apply_sound_changes(std::move(input), std::move(changes), std::move(stop_before));
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

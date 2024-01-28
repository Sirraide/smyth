#include <QJSEngine>
#include <QMessageBox>
#include <QShortcut>
#include <QRegularExpression>
#include <Smyth/Unicode.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/TextPreviewDialog.hh>
#include <ui_MainWindow.h>

namespace smyth::ui {
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
} // namespace smyth::ui

/// Needs destructor that isn’t visible in the header.
smyth::ui::MainWindow::~MainWindow() noexcept = default;

/// Only minimal initialisation here; the rest is done in `persist()`;
/// this is because some fields (e.g. `App::main_window()`) are only
/// initialised after this returns.
smyth::ui::MainWindow::MainWindow()
    : QMainWindow(nullptr),
      ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);

    /// Initialise shortcuts.
    auto save = new QShortcut(QKeySequence::Save, this);
    auto open = new QShortcut(QKeySequence::Open, this);
    connect(save, &QShortcut::activated, this, &MainWindow::save_project);
    connect(open, &QShortcut::activated, this, &MainWindow::open_project);
}

auto smyth::ui::MainWindow::ApplySoundChanges() -> Result<> {
    auto Norm = [](QComboBox* cbox, QString plain) -> Result<QString> {
        const auto norm = [cbox] {
            switch (cbox->currentIndex()) {
                default: return NormalisationForm::None;
                case 1: return NormalisationForm::NFC;
                case 2: return NormalisationForm::NFD;
            }
        }();

        auto normed = Try(Normalise(
            norm,
            icu::UnicodeString(reinterpret_cast<char16_t*>(plain.data()), i32(plain.size()))
        ));

        return QString::fromUtf16(normed.getBuffer(), normed.length());
    };

    auto input = Try(Norm(ui->sca_cbox_input_norm, ui->input->toPlainText()));
    auto changes = Try(Norm(ui->sca_cbox_changes_norm, ui->changes->toPlainText()));

    /// If javascript is enabled, find all instances of `§{}§` and replace them with
    /// the result of evaluating the javascript expression inside the braces.
    if (ui->sca_chbox_enable_javascript->isChecked())
        Try(EvaluateAndInterpolateJavaScript(changes));

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
        /// does not contain whitespace, then this is the name of a rule;
        /// take everything up to the first whitespace character.
        auto colon = line.indexOf(':');
        if (colon == -1) continue;
        auto ws = line.indexOf(QRegularExpression("\\s"));
        auto name = line.mid(0, ws == -1 ? colon : std::min(colon, ws));
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
    auto output = Try(App::The().apply_sound_changes(std::move(input), std::move(changes), std::move(stop_before)));
    ui->output->setPlainText(Try(Norm(ui->sca_cbox_output_norm, std::move(output))));
    return {};
}

auto smyth::ui::MainWindow::EvaluateAndInterpolateJavaScript(QString& changes) -> Result<> {
    QJSEngine js;
    qsizetype pos = 0;
    for (;;) {
        auto next = changes.indexOf("§{", pos);
        if (next == -1) break;
        auto end = changes.indexOf("}§", next);
        if (end == -1) break;
        auto expr = changes.mid(next + 2, end - next - 2);
        auto result = js.evaluate(expr);
        if (result.isError()) return Err(
            "Exception in JS Evaluation: {}\nWhile evaluating:\n{}",
            result.toString(),
            expr
        );

        /// Do not insert null or undefined.
        if (not result.isNull() and not result.isUndefined()) {
            auto str = result.toString();
            changes.replace(next, end - next + 2, str);
            pos = next + str.size();
        } else {
            changes.remove(next, end - next + 2);
            pos = next;
        }

    }
    return {};
}

void smyth::ui::MainWindow::HandleErrors(Result<> r) {
    if (r.is_err()) Error("{}", r.err().message);
}

void smyth::ui::MainWindow::apply_sound_changes() {
    HandleErrors(ApplySoundChanges());
}

void smyth::ui::MainWindow::closeEvent(QCloseEvent* event) {
    App::The().quit(event);
}

auto smyth::ui::MainWindow::mono_font() const -> const QFont& {
    return ui->changes->font();
}

void smyth::ui::MainWindow::new_project() {
    App::The().new_project();
}

void smyth::ui::MainWindow::open_project() {
    HandleErrors(App::The().open());
}

void smyth::ui::MainWindow::open_settings() {
    App::The().settings_dialog()->exec();
}

void smyth::ui::MainWindow::persist() {
    auto& app = App::The();

    /// Initialise persistent settings.
    ui->input->persist(app, "main.input");
    ui->changes->persist(app, "main.changes");
    ui->output->persist(app, "main.output");
    app.persist<&QWidget::size, [](QWidget* w, QSize s) { w->resize(s); }>("main.window.size", this);
    app.persist<&QSplitter::sizes, &QSplitter::setSizes>("main.sca.splitter.sizes", ui->sca_text_edits);
    app.persist<&QWidget::font, &QWidget::setFont>("charmap.font", ui->char_map);
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
    HandleErrors(app.load_last_open_project());

    /// Init MVC stuff.
    //ui->char_map->update();
}

void smyth::ui::MainWindow::preview_changes_after_eval() {
    auto changes = ui->changes->toPlainText();
    if (auto res = EvaluateAndInterpolateJavaScript(changes); res.is_err()) {
        Error("{}", res.err().message);
        return;
    }

    TextPreviewDialog::Show("Sound Changes: Preview", changes, ui->changes->font(), this);
}

void smyth::ui::MainWindow::save_project() {
    HandleErrors(App::The().save());
}

auto smyth::ui::MainWindow::serif_font() const -> const QFont& {
    return ui->input->font();
}

void smyth::ui::MainWindow::set_mono_font(QFont f) {
    /// Set only the family and keep size etc. as is.
    QFont font{ui->changes->font()};
    font.setFamily(f.family());
    ui->changes->setFont(font);
}

void smyth::ui::MainWindow::set_serif_font(QFont f) {
    /// Set only the family and keep size etc. as is.
    QFont font{ui->input->font()};
    font.setFamily(f.family());
    ui->input->setFont(font);
    ui->output->setFont(font);

    /// The font size of this one is larger.
    QFont ctab_font{ui->char_map->font()};
    ctab_font.setFamily(f.family());
    ui->char_map->setFont(ctab_font);
    ui->char_map->update();
}

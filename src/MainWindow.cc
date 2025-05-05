#include <base/Text.hh>
#include <print>
#include <QFileDialog>
#include <QJSEngine>
#include <QShortcut>
#include <UI/Lexurgy.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/TextPreviewDialog.hh>
#include <ui_MainWindow.h>

using namespace smyth;
using namespace smyth::ui;

MainWindow* MainWindow::Instance;

// ====================================================================
//  Initialisation
// ====================================================================
/// Only minimal initialisation here; the rest is done in `persist()`;
/// this is because some fields (e.g. `App::main_window()`) are only
/// initialised after this returns.
MainWindow::MainWindow()
    : QMainWindow(nullptr),
      ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);
    setWindowTitle("Smyth");

    // Initialise shortcuts.
    auto save = new QShortcut(QKeySequence::Save, this);
    auto open = new QShortcut(QKeySequence::Open, this);
    auto quit = new QShortcut(QKeySequence::Quit, this);
    connect(save, &QShortcut::activated, this, &MainWindow::save_project);
    connect(open, &QShortcut::activated, this, &MainWindow::open_project);
    connect(quit, &QShortcut::activated, this, &MainWindow::close); // FIXME: Should prompt for save

    // Initialise other signals.
    connect(ui->char_map, &SmythCharacterMap::selected, this, &MainWindow::char_map_update_selection);
}

void MainWindow::Init() {
    // Initialise notes tab.
    ui->notes_file_list->init();

    // FIXME: I think we need to set the window size before
    // doing any of this here.

    // Show the window to force widgets to render.
    show();

    // Focus all tabs to initialise everything.
    for (int i = 0; i < ui->main_tabs->count(); i++) {
        ui->main_tabs->setCurrentIndex(i);
        layout()->invalidate();
    }

    // Switch to first tab and initialise persistent state.
    ui->main_tabs->setCurrentIndex(0);
    Persist();
}

void MainWindow::Persist() {
    PersistentStore& main_store = PersistentStore::Create("main");

    // Window needs to be updated before everything else to ensure that
    // the rest of the objects are working with the correct size.
    Persist<&QWidget::size, [](QWidget* w, QSize s) {
        w->resize(s);
        QApplication::processEvents();
    }>(main_store, "window.size", this, 1);

    // Initialise persistent settings.
    ui->input->persist(main_store, "input");
    ui->changes->persist(main_store, "changes");
    ui->output->persist(main_store, "output");

    PersistentStore& charmap = PersistentStore::Create("charmap", main_store);
    PersistState(charmap, "splitter.sizes", ui->char_map_splitter);

    PersistentStore& sca = PersistentStore::Create("sca", main_store);
    PersistState(sca, "splitter.sizes", ui->sca_text_edits);
    PersistCBox(sca, "cbox.input.norm.choice", ui->sca_cbox_input_norm);
    PersistCBox(sca, "cbox.changes.norm.choice", ui->sca_cbox_changes_norm);
    PersistCBox(sca, "cbox.output.norm.choice", ui->sca_cbox_output_norm);
    PersistDynCBox(sca, "cbox.start.after", ui->sca_cbox_start_after);
    PersistDynCBox(sca, "cbox.stop.before", ui->sca_cbox_stop_before);
    PersistChBox(sca, "chbox.details", ui->sca_chbox_details);
    PersistChBox(sca, "chbox.enable.js", ui->sca_chbox_enable_javascript);

    PersistentStore& notes_store = PersistentStore::Create("notes", main_store);
    ui->notes_file_list->persist(notes_store);
    PersistState(notes_store, "splitter.sizes", ui->notes_splitter);

    PersistentStore& dictionary_store = PersistentStore::Create("dictionary", main_store);
    ui->dictionary_table->persist(dictionary_store);

    PersistentStore& wordgen_store = PersistentStore::Create("wordgen", main_store);
    ui->wordgen_classes_input->persist(wordgen_store, "classes");
    ui->wordgen_output->persist(wordgen_store, "output");
    Persist<&QLineEdit::text, &QLineEdit::setText>(wordgen_store, "phono", ui->wordgen_input_phono);
    PersistState(wordgen_store, "splitter", ui->wordgen_splitter);

    // Hide the details panels if the checkbox is unchecked.
    if (not ui->sca_chbox_details->isChecked()) {
        ui->sca_frame_input_bottom->setVisible(false);
        ui->sca_frame_changes_bottom->setVisible(false);
        ui->sca_frame_output_bottom->setVisible(false);
        ui->frame_start_after->setVisible(false);
        ui->frame_stop_before->setVisible(false);
    }

    // Init user settings.
    settings::SerifFont.subscribe(ui->input, &SmythPlainTextEdit::setFont);
    settings::SerifFont.subscribe(ui->output, &SmythPlainTextEdit::setFont);
    settings::SerifFont.subscribe(ui->char_map, &SmythCharacterMap::setFont);
    settings::SerifFont.subscribe(ui->char_map_details_panel, &SmythRichTextEdit::setFont);
    settings::SerifFont.subscribe(ui->wordgen_classes_input, &SmythPlainTextEdit::setFont);
    settings::SerifFont.subscribe(ui->wordgen_output, &SmythPlainTextEdit::setFont);
    settings::SerifFont.subscribe(ui->wordgen_input_phono, &QLineEdit::setFont);
    settings::MonoFont.subscribe(ui->changes, &SmythPlainTextEdit::setFont);
    settings::SansFont.subscribe(ui->notes_text_box, &SmythRichTextEdit::setFont);
    settings::LastOpenProject.subscribe([](const QString& s) { SetWindowPath(s); });
}

// ====================================================================
//  API
// ====================================================================
/// Needs destructor that isn’t visible in the header.
MainWindow::~MainWindow() noexcept = default;

auto MainWindow::GetNotesTabTextBox() -> SmythPlainTextEdit* {
    return Instance->ui->notes_text_box;
}

auto MainWindow::Prompt(
    const QString& title,
    const QString& message,
    QMessageBox::StandardButtons buttons
) -> QMessageBox::StandardButton {
    return QMessageBox::question(Instance, title, message, buttons);
}

void MainWindow::Reset() {
    Instance->ui->dictionary_table->reset_dictionary();
    SetWindowPath("");
}

void MainWindow::SetWindowPath(QString path) {
    if (path.isEmpty()) {
        Instance->setWindowFilePath("");
        Instance->setWindowTitle("Smyth");
        return;
    }

    // Strip home directory from path.
#ifdef __linux__
    auto home_path = QDir::homePath();
    if (path.startsWith(home_path)) path = "~" + path.mid(home_path.size());
#endif

    Instance->setWindowFilePath(path);
    Instance->setWindowTitle(QString::fromStdString(std::format("Smyth | {}", path.toStdString())));
}

auto MainWindow::ShowError(
    const QString& error,
    QMessageBox::StandardButtons buttons,
    const QString& title
) -> QMessageBox::StandardButton {
    return QMessageBox::critical(Instance, title, error, buttons);
}

// ====================================================================
//  Internals
// ====================================================================
auto MainWindow::ApplySoundChanges() -> Result<> {
    auto Norm = [](QComboBox* cbox, QString plain) -> Result<QString> {
        const auto norm = [cbox] {
            switch (cbox->currentIndex()) {
                default: return text::NormalisationForm::None;
                case 1: return text::NormalisationForm::NFC;
                case 2: return text::NormalisationForm::NFD;
            }
        }();

        return QString::fromStdString(Normalise(plain.toStdString(), norm));
    };

    auto input = Try(Norm(ui->sca_cbox_input_norm, ui->input->toPlainText()));
    auto changes = Try(Norm(ui->sca_cbox_changes_norm, ui->changes->toPlainText()));

    // If javascript is enabled, find all instances of `§{}§` and replace them with
    // the result of evaluating the javascript expression inside the braces.
    if (ui->sca_chbox_enable_javascript->isChecked())
        Try(EvaluateAndInterpolateJavaScript(changes));

    // Remember the 'Stop Before' rule that is currently selected.
    auto start_after = ui->sca_cbox_start_after->currentText();
    auto stop_before = ui->sca_cbox_stop_before->currentText();

    // Parse the sound changes to figure out what rules we have for the
    // 'Stop Before' dropdown; rule names are lines that start with a name
    // followed by a colon.
    std::vector<QString> rule_names;
    for (auto line : changes.split('\n')) {
        line = line.trimmed();
        if (
            line.isEmpty() or
            line.startsWith('#') or
            line.startsWith('\\') or
            line.contains("=>")
        ) continue;

        // If the line contains a colon, and everything before the colon
        // does not contain whitespace, then this is the name of a rule;
        // take everything up to the first whitespace character.
        auto colon = line.indexOf(':');
        if (colon == -1) continue;
        auto ws = line.indexOf(QRegularExpression("\\s"));
        auto name = line.mid(0, ws == -1 ? colon : std::min(colon, ws));
        rule_names.push_back(name);
    }

    // Filter out special rules.
    std::erase_if(rule_names, [](const QString& str) {
        return str.startsWith("romanizer") or
               str == "Then" or
               str == "deromanizer" or
               str == "Syllables";
    });

    // Update the 'Start After'/'Stop Before' dropdowns.
    ui->sca_cbox_start_after->clear();
    ui->sca_cbox_start_after->addItem("");
    ui->sca_cbox_stop_before->clear();
    ui->sca_cbox_stop_before->addItem("");
    for (const auto& name : rule_names) {
        ui->sca_cbox_start_after->addItem(name);
        ui->sca_cbox_stop_before->addItem(name);
    }

    // If the rule that was selected before still exists, select it again; if
    // not, do not stop before any rule.
    if (rgs::contains(rule_names, start_after)) ui->sca_cbox_start_after->setCurrentText(start_after);
    else start_after = "";
    if (rgs::contains(rule_names, stop_before)) ui->sca_cbox_stop_before->setCurrentText(stop_before);
    else stop_before = "";

    // Dew it.
    auto output = Try(smyth::lexurgy::Apply(input, std::move(changes), start_after, stop_before));
    ui->output->setPlainText(Try(Norm(ui->sca_cbox_output_norm, std::move(output))));
    return {};
}

auto MainWindow::EvaluateAndInterpolateJavaScript(QString& changes) -> Result<> {
    QJSEngine js;
    js.installExtensions(QJSEngine::ConsoleExtension);
    qsizetype pos = 0;
    for (;;) {
        auto next = changes.indexOf("§{", pos);
        if (next == -1) break;
        auto end = changes.indexOf("}§", next);
        if (end == -1) break;
        auto expr = changes.mid(next + 2, end - next - 2);
        auto result = js.evaluate(expr);
        if (result.isError()) return Error(
            "Exception in JS Evaluation: {}\nWhile evaluating:\n{}",
            result.toString(),
            expr
        );

        // Do not insert null or undefined.
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

// ====================================================================
//  Slots
// ====================================================================
void MainWindow::apply_sound_changes() {
    HandleErrors(ApplySoundChanges());
}

void MainWindow::char_map_update_selection(char32_t codepoint) {
    static constexpr auto LC = text::CharCategory::LowercaseLetter;
    static constexpr auto UC = text::CharCategory::UppercaseLetter;
    static constexpr std::string_view templ = R"html(
        <h2>U+{:04X}</h2>
        <h4>{}</h4>
        {}
    )html";
    c32 c = codepoint;

    auto cat = c.category();
    std::string swap_case =
        cat == LC   ? std::format("Uppercase: U+{:04X}", c.to_upper())
        : cat == UC ? std::format("Lowercase: U+{:04X}", c.to_lower())
                    : "";

    auto html = std::format(
        templ,
        u32(c),
        c.name().value_or("<error retrieving char name>"),
        swap_case.empty() ? "" : std::format("<p><strong>{}</strong></p>", swap_case)
    );

    ui->char_map_details_panel->setHtml(QString::fromStdString(html));
}

void MainWindow::generate_words() {
    std::println("Classes: {}", ui->wordgen_classes_input->toPlainText());
    std::println("Phono: {}", ui->wordgen_input_phono->text());
}

void MainWindow::new_project() {
    Project::New();
}

void MainWindow::open_project() {
    auto path = QFileDialog::getOpenFileName(
        nullptr,
        "Open Project",
        "",
        "Smyth Projects (*.smyth)"
    );

    if (path.isEmpty()) return;
    Project::Open(std::move(path));
}

void MainWindow::open_settings() {
    SettingsDialog::Exec();
}

void MainWindow::preview_changes_after_eval() {
    auto changes = ui->changes->toPlainText();
    if (auto res = EvaluateAndInterpolateJavaScript(changes); not res) {
        HandleErrors(std::move(res));
        return;
    }

    TextPreviewDialog::Show("Sound Changes: Preview", changes, ui->changes->font(), this);
}

void MainWindow::prompt_quit() {
    if (not Project::PromptClose()) return;
    close();
}

void MainWindow::save_project() {
    Project::Save();
}

void MainWindow::show_project_directory() {
    Project::OpenDirInNativeShell();
}

// ====================================================================
//  Events
// ====================================================================
void MainWindow::closeEvent(QCloseEvent* event) {
    if (not Project::PromptClose()) return event->ignore();
    QMainWindow::closeEvent(event);
}

#include <QDir>
#include <QInputDialog>
#include <QJSEngine>
#include <QMessageBox>
#include <QRegularExpression>
#include <QShortcut>
#include <Smyth/Unicode.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/TextPreviewDialog.hh>
#include <ui_MainWindow.h>

/// Needs destructor that isn’t visible in the header.
smyth::ui::MainWindow::~MainWindow() noexcept = default;

/// Only minimal initialisation here; the rest is done in `persist()`;
/// this is because some fields (e.g. `App::main_window()`) are only
/// initialised after this returns.
smyth::ui::MainWindow::MainWindow()
    : QMainWindow(nullptr),
      ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);

    // Initialise shortcuts.
    auto save = new QShortcut(QKeySequence::Save, this);
    auto open = new QShortcut(QKeySequence::Open, this);
    auto quit = new QShortcut(QKeySequence::Quit, this);
    connect(save, &QShortcut::activated, this, &MainWindow::save_project);
    connect(open, &QShortcut::activated, this, &MainWindow::open_project);
    connect(quit, &QShortcut::activated, this, &MainWindow::close); // FIXME: Should prompt for save

    // Initialise other signals.
    connect(ui->char_map, &SmythCharacterMap::selected, this, &MainWindow::char_map_update_selection);

    // Call debug() when F12 is pressed.
    SMYTH_DEBUG(
        auto debug = new QShortcut(QKeySequence(Qt::Key_F12), this);
        connect(debug, &QShortcut::activated, this, &MainWindow::debug);
    )
}

auto smyth::ui::MainWindow::ApplySoundChanges() -> Result<> {
    auto Norm = [](QComboBox* cbox, QString plain) -> Result<QString> {
        const auto norm = [cbox] {
            switch (cbox->currentIndex()) {
                default: return unicode::NormalisationForm::None;
                case 1: return unicode::NormalisationForm::NFC;
                case 2: return unicode::NormalisationForm::NFD;
            }
        }();

        auto normed = Try(unicode::Normalise(
            norm,
            icu::UnicodeString(reinterpret_cast<char16_t*>(plain.data()), i32(plain.size()))
        ));

        return QString::fromUtf16(normed.getBuffer(), normed.length());
    };

    auto input = Try(Norm(ui->sca_cbox_input_norm, ui->input->toPlainText()));
    auto changes = Try(Norm(ui->sca_cbox_changes_norm, ui->changes->toPlainText()));

    // If javascript is enabled, find all instances of `§{}§` and replace them with
    // the result of evaluating the javascript expression inside the braces.
    if (ui->sca_chbox_enable_javascript->isChecked())
        Try(EvaluateAndInterpolateJavaScript(changes));

    // Remember the 'Stop Before' rule that is currently selected.
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

    // Update the 'Stop Before' dropdown.
    ui->sca_cbox_stop_before->clear();
    ui->sca_cbox_stop_before->addItem("");
    for (const auto& name : rule_names) ui->sca_cbox_stop_before->addItem(name);

    // If the rule that was selected before still exists, select it again; if
    // not, do not stop before any rule.
    if (rgs::contains(rule_names, stop_before)) ui->sca_cbox_stop_before->setCurrentText(stop_before);
    else stop_before = "";

    // Dew it.
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

void smyth::ui::MainWindow::HandleErrors(Result<> r) {
    if (r.is_err()) Error("{}", r.err().message);
}

void smyth::ui::MainWindow::apply_sound_changes() {
    HandleErrors(ApplySoundChanges());
}

void smyth::ui::MainWindow::char_map_update_selection(char32_t codepoint) {
    static constexpr auto LC = U_LOWERCASE_LETTER;
    static constexpr auto UC = U_UPPERCASE_LETTER;
    static constexpr std::string_view templ = R"html(
        <h2>U+{:04X}</h2>
        <h4>{}</h4>
        {}
    )html";
    c32 c = codepoint;

    auto cat = c.category();
    std::string swap_case =
        cat == LC   ? fmt::format("Uppercase: U+{:04X}", c.to_upper())
        : cat == UC ? fmt::format("Lowercase: U+{:04X}", c.to_lower())
                    : "";

    auto html = fmt::format(
        templ,
        u32(c),
        c.name().value_or("<error retrieving char name>"),
        swap_case.empty() ? "" : fmt::format("<p><strong>{}</strong></p>", swap_case)
    );

    ui->char_map_details_panel->setHtml(QString::fromStdString(html));
}

void smyth::ui::MainWindow::closeEvent(QCloseEvent* event) {
    if (not App::The().prompt_close_project()) return event->ignore();
    QMainWindow::closeEvent(event);
}

void smyth::ui::MainWindow::debug() {
    fmt::print("SIZE: {}\n", ui->char_map_splitter->sizes());
}

void smyth::ui::MainWindow::init() {
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
    persist();
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

    // Window needs to be updated before everything else to ensure that
    // the rest of the objects are working with the correct size.
    app.persist<&QWidget::size, [](QWidget* w, QSize s) {
        w->resize(s);
        QApplication::processEvents();
    }>("main.window.size", this, 1);

    // Initialise persistent settings.
    ui->input->persist(app, "main.input");
    ui->changes->persist(app, "main.changes");
    ui->output->persist(app, "main.output");
    ui->char_map_details_panel->persist(app, "charmap.details", false);
    app.persist<&QWidget::font, &QWidget::setFont>("charmap.font", ui->char_map);
    PersistSplitter("main.sca.splitter.sizes", ui->sca_text_edits);
    PersistSplitter("charmap.splitter.sizes", ui->char_map_splitter);
    PersistCBox("main.sca.cbox.input.norm.choice", ui->sca_cbox_input_norm);
    PersistCBox("main.sca.cbox.changes.norm.choice", ui->sca_cbox_changes_norm);
    PersistCBox("main.sca.cbox.output.norm.choice", ui->sca_cbox_output_norm);
    PersistDynCBox("main.sca.cbox.stop.before", ui->sca_cbox_stop_before);
    PersistChBox("main.sca.chbox.details", ui->sca_chbox_details);
    PersistChBox("main.sca.chbox.enable.js", ui->sca_chbox_enable_javascript);

    // Hide the details panels if the checkbox is unchecked.
    if (not ui->sca_chbox_details->isChecked()) {
        ui->sca_frame_input_bottom->setVisible(false);
        ui->sca_frame_changes_bottom->setVisible(false);
        ui->sca_frame_output_bottom->setVisible(false);
        ui->frame_stop_before->setVisible(false);
    }

    // Load last open project, if any.
    HandleErrors(app.load_last_open_project());
}

void smyth::ui::MainWindow::preview_changes_after_eval() {
    auto changes = ui->changes->toPlainText();
    if (auto res = EvaluateAndInterpolateJavaScript(changes); res.is_err()) {
        Error("{}", res.err().message);
        return;
    }

    TextPreviewDialog::Show("Sound Changes: Preview", changes, ui->changes->font(), this);
}

void smyth::ui::MainWindow::prompt_quit() {
    if (not App::The().prompt_close_project()) return;
    close();
}

void smyth::ui::MainWindow::save_project() {
    HandleErrors(App::The().save());
}

auto smyth::ui::MainWindow::serif_font() const -> const QFont& {
    return ui->input->font();
}

void smyth::ui::MainWindow::set_mono_font(QFont f) {
    // Set only the family and keep size etc. as is.
    QFont font{ui->changes->font()};
    font.setFamily(f.family());
    ui->changes->setFont(font);
}

void smyth::ui::MainWindow::set_serif_font(QFont f) {
    // Set only the family and keep size etc. as is.
    QFont font{ui->input->font()};
    font.setFamily(f.family());
    ui->input->setFont(font);
    ui->output->setFont(font);

    // The font size of this one is larger.
    QFont ctab_font{ui->char_map->font()};
    ctab_font.setFamily(f.family());
    ui->char_map->setFont(ctab_font);
    ui->char_map->update();
}

void smyth::ui::MainWindow::set_window_path(QString path) {
    if (path.isEmpty()) {
        setWindowFilePath("");
        setWindowTitle("Smyth");
        return;
    }

    // Strip home directory from path.
#ifdef __linux__
    auto home_path = QDir::homePath();
    if (path.startsWith(home_path)) path = "~" + path.mid(home_path.size());
#endif

    setWindowFilePath(path);
    setWindowTitle(QString::fromStdString(fmt::format("Smyth | {}", path.toStdString())));
}


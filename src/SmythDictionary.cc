#include <base/Defer.hh>
#include <base/FS.hh>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <Smyth/JSON.hh>
#include <UI/App.hh>
#include <UI/MainWindow.hh>
#include <UI/SettingsDialog.hh>
#include <UI/SmythDictionary.hh>
#include <ui_CSVImportExportDialog.h>

using namespace smyth;
using namespace smyth::json_utils;
using namespace smyth::ui;
using smyth::detail::Serialiser;

namespace {
struct PersistColumns : smyth::detail::PersistentBase {
    SmythDictionary* dict;
    PersistColumns(SmythDictionary* dict) : dict{dict} {}

    auto load(const json& j) -> Result<> override;
    void restore() override;
    auto save() const -> Result<json> override;
};

struct PersistContents : smyth::detail::PersistentBase {
    SmythDictionary* dict;
    PersistContents(SmythDictionary* dict) : dict{dict} {}

    auto load(const json& j) -> Result<> override;
    void restore() override;
    auto save() const -> Result<json> override;
};
} // namespace

void SmythDictionary::debug() {
}

/// ====================================================================
///  Dictionary
/// ====================================================================
SmythDictionary::~SmythDictionary() = default;
SmythDictionary::SmythDictionary(QWidget* parent)
    : QTableWidget(parent),
      import_dialog(false, App::MainWindow()),
      export_dialog(true, App::MainWindow()) {
    setAlternatingRowColors(true);

    // Set up the context menu.
    context_menu = new QMenu(this);
    auto add_row = context_menu->addAction("Add Row");
    auto add_column = context_menu->addAction("Add Column");
    context_menu->addSeparator();
    auto delete_rows = context_menu->addAction("Delete Rows");
    auto delete_columns = context_menu->addAction("Delete Columns");
    context_menu->addSeparator();
    auto duplicate_entry = context_menu->addAction("Duplicate Entry");
    duplicate_entry->setShortcut(Qt::CTRL | Qt::Key_Return);
    delete_rows->setShortcut(Qt::Key_Delete);
    add_row->setShortcut(Qt::CTRL | Qt::Key_N);
    connect(add_row, &QAction::triggered, this, &SmythDictionary::add_row);
    connect(add_column, &QAction::triggered, this, &SmythDictionary::add_column);
    connect(delete_rows, &QAction::triggered, this, &SmythDictionary::delete_rows);
    connect(delete_columns, &QAction::triggered, this, &SmythDictionary::delete_columns);
    connect(duplicate_entry, &QAction::triggered, this, &SmythDictionary::duplicate_entry);

    // Update font when it changes.
    App::The().serif_font.subscribe(this, &SmythDictionary::setFont);

    // Make horizontal header movable and ensure that the last column
    // stretches to fill the remaining space. Also make it editable.
    setHorizontalHeader(new detail::ColumnHeaders{this});
    auto hhdr = horizontalHeader();
    hhdr->setSectionsMovable(true);
    hhdr->setStretchLastSection(true);
    hhdr->setCascadingSectionResizes(true);

    // Set up signals.
    connect(hhdr, &detail::ColumnHeaders::sectionMoved, this, &SmythDictionary::ActuallyMoveTheDamnableSection);

    // Make vertical header non-movable and ensure that each line has the
    // same height, based on the font metrics.
    auto vhdr = verticalHeader();
    vhdr->setSectionsMovable(false);
    vhdr->setDefaultSectionSize(fontMetrics().height());
    vhdr->setSectionResizeMode(QHeaderView::Fixed);

    // Create an empty dictionary.
    reset_dictionary();
}

// Because of SOME UNGODLY REASON, Qt has a concept of ‘logical’
// and ‘visual’ sections. When a section is moved, it doesn’t
// *actually* move the section. Rather, the section is internally
// marked as ‘displayed somewhere different from where it actually
// is’, which wreaks HAVOC on any code trying to map indices to
// sections. Fix that by ACTUALLY MOVING THE GOD DAMN SECTION FFS.
void SmythDictionary::ActuallyMoveTheDamnableSection(int, int old_vis, int new_vis) {
    if (section_move_fixup_running) return;
    tempset section_move_fixup_running = true;

    // First, undo the move.
    horizontalHeader()->moveSection(new_vis, old_vis);

    // Check someone else hasn’t screwed us over.
    for (int col = 0; col < columnCount(); col++)
        Assert(col == visualColumn(col), "Bad column index: {}", col);

    // Then, exchange the cell contents.
    for (auto row = 0; row < rowCount(); row++) {
        auto cell1 = takeItem(row, old_vis);
        auto cell2 = takeItem(row, new_vis);
        setItem(row, old_vis, cell2);
        setItem(row, new_vis, cell1);
    }

    // And the cell headers.
    auto header1 = takeHorizontalHeaderItem(old_vis);
    auto header2 = takeHorizontalHeaderItem(new_vis);
    setHorizontalHeaderItem(old_vis, header2);
    setHorizontalHeaderItem(new_vis, header1);
}

void SmythDictionary::DeleteSelectedColumns() {
    // Figure out what rows we’re supposed to delete.
    std::vector<int> cols;
    for (auto rng : selectedRanges())
        for (int col = rng.leftColumn(); col <= rng.rightColumn(); ++col)
            cols.push_back(col);

    // Prompt the user to delete the rows.
    if (not Prompt("Deleting Columns", "Are you sure you want to delete {} columns(s)?", cols.size())) return;
    rgs::sort(cols, std::greater<int>{});
    for (auto col : cols) removeColumn(col);

    // If we end up with no rows as a result, insert a new one.
    if (columnCount() == 0) add_column();
}

auto SmythDictionary::DuplicateSelectedEntry() -> Result<> {
    if (selectedRanges().empty()) return {};

    // Duplicate the first selected row.
    auto first_row = selectedRanges().front().topRow();
    insertRow(first_row + 1);

    // Copy all cells that we were told to select.
    auto cols = Try(App::The().settings_dialog()->get_rows_to_duplicate());
    for (int col = 0; col < columnCount(); ++col) {
        auto it = item(first_row, col);
        if (not it) continue;
        if (not cols.empty() and not cols.contains(col + 1)) continue;
        setItem(first_row + 1, col, new QTableWidgetItem(it->text()));
    }

    return {};
}

void SmythDictionary::DeleteSelectedRows() {
    // Figure out what rows we’re supposed to delete.
    std::vector<int> rows;
    for (auto rng : selectedRanges())
        for (int row = rng.topRow(); row <= rng.bottomRow(); ++row)
            rows.push_back(row);

    // Prompt the user to delete the rows.
    if (not Prompt("Deleting Rows", "Are you sure you want to delete {} row(s)?", rows.size())) return;
    rgs::sort(rows, std::greater<int>{});
    for (auto row : rows) removeRow(row);

    // If we end up with no rows as a result, insert a new one.
    if (rowCount() == 0) add_row();
}

auto SmythDictionary::ExportCSV() -> Result<> {
    return {};
    /*auto path = QFileDialog::getSaveFileName(
        this,
        "Export Dictionary",
        QString{},
        "Comma Separated Values (*.csv)"
    );

    if (path.isEmpty()) return {};
    bool allow_multiline = export_dialog->check_multi_line->isChecked();

    // Separator must not be empty.
    auto separator = export_dialog->text_separator->text().trimmed();
    if (separator.isEmpty()) return Error("Separator must not be empty.");

    // Likewise for the delimiter if the 'Enclosed by' checkbox is checked
    bool has_delim = export_dialog->check_enclosed_by->isChecked();
    QString delim;
    if (has_delim) {
        delim = export_dialog->text_enclosed_by->text().trimmed();
        if (delim.isEmpty()) return Error("Delimiter must not be empty.");
    }

    // Helper to append escaped text.
    QString csv;

    // Include header if requested.
    if (export_dialog->check_header_line->isChecked()) {
        for (int col = 0; col < columnCount(); ++col) {
            if (col != 0) csv += ',';
            auto it = horizontalHeaderItem(col);
            if (it) csv += it->text();
        }
        csv += '\n';
    }

    // Write each row.
    for (int row = 0; row < rowCount(); ++row) {
        for (int col = 0; col < columnCount(); ++col) {
            if (col != 0) csv += ',';

            // Ignore empty items.
            auto it = item(row, col);
            if (not it) continue;
            auto text = it->text().trimmed();

            // If the field starts with an odd number of delimiters, add one.
            if (has_delim) {
                QStringView sv = text;
                usz delim_count = 0;
                while (sv.startsWith(delim)) {
                    ++delim_count;
                    sv = sv.mid(delim.size());
                }
                if (delim_count % 2 == 1) csv += delim;
            }

            // Check if it contains the separator.
            bool contains_separator = text.contains(separator);
            bool contains_newline = text.contains('\n');
            if (contains_separator or contains_newline) {
                // Can’t proceed if the user disallowed delimiters.
                if (not has_delim) return Error(
                    "Field '{}' in row '{}' contains {}. Specify "
                    "an enclosing delimiter to proceed.\n\nField content:\n{}",
                    col + 1,
                    row + 1,
                    contains_separator ? "the separator '" + separator + "'" : "a newline",
                    separator,
                    text
                );

                // If the field contains a newline, we also can’t proceed
                // if multiline fields are disallowed.
                if (contains_newline and not allow_multiline) return Error(
                    "Field '{}' in row '{}' contains a newline, but "
                    "multi-line fields are disallowed.",
                    col + 1,
                    row + 1,
                    text
                );

                // Enclose the text in delimiters.
                csv += delim;
                csv += text;
                csv += delim;

                // If this causes the CSV to end with an even number of
                // delimiters, we need to add one more.
                QStringView sv = csv;
                usz delim_count = 0;
                while (sv.endsWith(delim)) {
                    ++delim_count;
                    sv = sv.left(sv.size() - delim.size());
                }
                if (delim_count % 2 == 0) csv += delim;
            }

            // Otherwise, write out the text as is.
            else { csv += text; }
        }

        csv += '\n';
    }*/
}

auto SmythDictionary::ImportCSV(bool replace) -> Result<> {
    return {};
    /*auto file = QFileDialog::getOpenFileName(
        this,
        "Import Dictionary",
        QString{},
        "Comma Separated Values (*.csv)"
    );

    if (file.isEmpty()) return {};*/
}

void SmythDictionary::add_column() {
    insertColumn(columnCount());
}

void SmythDictionary::add_row() {
    insertRow(rowCount());
}

void SmythDictionary::delete_columns(bool) {
    DeleteSelectedColumns();
}

void SmythDictionary::delete_rows(bool) {
    DeleteSelectedRows();
}

void SmythDictionary::duplicate_entry(bool) {
    App::MainWindow()->HandleErrors(DuplicateSelectedEntry());
}

void SmythDictionary::contextMenuEvent(QContextMenuEvent* event) {
    context_menu->popup(mapToGlobal(event->pos()));
}

void SmythDictionary::export_dictionary() {
    App::The().MainWindow()->HandleErrors(ExportCSV());
}

void SmythDictionary::keyPressEvent(QKeyEvent* event) {
    if (HandleZoomEvent(event)) return;

    // Prompt user to delete selected row.
    if (state() == NoState) {
        if (event->key() == Qt::Key_Delete and not selectedRanges().empty()) {
            DeleteSelectedRows();
            return;
        }

        if (event->modifiers() & Qt::ControlModifier and event->key() == Qt::Key_Return) {
            App::MainWindow()->HandleErrors(DuplicateSelectedEntry());
            return;
        }

        if (event->modifiers() & Qt::ControlModifier and event->key() == Qt::Key_N) {
            add_row();
            return;
        }
    }

    QTableWidget::keyPressEvent(event);
}

void SmythDictionary::import() {
    App::The().MainWindow()->HandleErrors(ImportCSV(false));
}

void SmythDictionary::import_and_replace() {
    App::The().MainWindow()->HandleErrors(ImportCSV(true));
}

void SmythDictionary::reset_dictionary() {
    clear();
    add_row();
    setRowCount(1);
    setColumnCount(2);
}

/// ====================================================================
///  Column Headers
/// ====================================================================
void smyth::ui::detail::ColumnHeaders::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QHeaderView::mouseDoubleClickEvent(event);
        return;
    }

    // Map position to index.
    auto index = logicalIndexAt(event->pos());
    if (index == -1) return;

    // Edit this cell.
    auto text = QInputDialog::getText(
        this,
        "Edit",
        "New column name:",
        QLineEdit::Normal,
        model()->headerData(index, orientation()).toString()
    );

    // Update the header.
    if (text.isEmpty()) return;
    static_cast<SmythDictionary*>(parent())->setHorizontalHeaderItem(
        index,
        new QTableWidgetItem{text}
    );
}

/// ====================================================================
///  Persistence
/// ====================================================================
void SmythDictionary::persist(PersistentStore& store) {
    // Load columns first so we can set the column count; otherwise, any
    // out-of-bounds assignments to cells in non-existent columns will
    // silently get dropped.
    store.register_entry("columns", {std::make_unique<PersistColumns>(this), 1});
    store.register_entry("contents", std::make_unique<PersistContents>(this));
}

auto PersistColumns::load(const json& j) -> Result<> {
    const json::array_t& arr = Try(Get<json::array_t>(j));

    // No entries; set to defaults.
    if (arr.empty()) {
        dict->setColumnCount(2);
        return {};
    }

    // Set the column count and the name of each column.
    dict->setColumnCount(int(arr.size()));
    for (auto [index, e] : arr | vws::enumerate) {
        auto text = Try(Serialiser<QString>::Deserialise(e));
        if (text.isEmpty()) continue;
        dict->setHorizontalHeaderItem(int(index), new QTableWidgetItem{text});
    }
    return {};
}

auto PersistContents::load(const json& j) -> Result<> {
    const json::array_t& arr = Try(Get<json::array_t>(j));

    // No entries; set to defaults.
    if (arr.empty()) {
        restore();
        return {};
    }

    // Get column count from first row.
    dict->setRowCount(int(arr.size()));
    usz col_count = 0;

    // Add the rows.
    for (auto [row_index, e] : arr | vws::enumerate) {
        const json::array_t& row = Try(Get<json::array_t>(e));
        col_count = std::max(col_count, row.size());
        for (auto [col_index, col] : row | vws::enumerate) {
            auto text = Try(Serialiser<QString>::Deserialise(col));
            if (text.isEmpty()) continue;
            dict->setItem(int(row_index), int(col_index), new QTableWidgetItem{text});
        }
    }
    return {};
}

void PersistColumns::restore() {
    dict->setColumnCount(2);
}

void PersistContents::restore() {
    dict->reset_dictionary();
}

auto PersistColumns::save() const -> Result<json> {
    json::array_t cols;
    for (int col = 0; col < dict->columnCount(); ++col) {
        auto it = dict->horizontalHeaderItem(col);
        if (it) cols.push_back(Serialiser<QString>::Serialise(it->text()));
        else cols.push_back("");
    }
    return std::move(cols);
}

auto PersistContents::save() const -> Result<json> {
    json::array_t rows;
    for (int row = 0; row < dict->rowCount(); ++row) {
        json::array_t cols;
        for (int col = 0; col < dict->columnCount(); ++col) {
            auto it = dict->item(row, col);
            if (it) cols.push_back(Serialiser<QString>::Serialise(it->text()));
            else cols.push_back("");
        }
        rows.push_back(std::move(cols));
    }
    return std::move(rows);
}

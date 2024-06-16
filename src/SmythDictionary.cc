#include <base/FS.hh>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <Smyth/JSON.hh>
#include <UI/App.hh>
#include <UI/MainWindow.hh>
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
export_dialog(true, App::MainWindow())
{
    setAlternatingRowColors(true);

    // Set up the context menu.
    context_menu = new QMenu(this);
    auto add_row = context_menu->addAction("Add Row");
    auto add_column = context_menu->addAction("Add Column");
    auto delete_rows = context_menu->addAction("Delete Rows");
    auto delete_columns = context_menu->addAction("Delete Columns");
    connect(add_row, &QAction::triggered, this, &SmythDictionary::add_row);
    connect(add_column, &QAction::triggered, this, &SmythDictionary::add_column);
    connect(delete_rows, &QAction::triggered, this, &SmythDictionary::delete_rows);
    connect(delete_columns, &QAction::triggered, this, &SmythDictionary::delete_columns);

    // Update font when it changes.
    App::The().serif_font.subscribe(this, &SmythDictionary::setFont);

    // Make horizontal header non-movable and ensure that the last column
    // stretches to fill the remaining space. Also make it editable.
    setHorizontalHeader(new detail::ColumnHeaders{this});
    auto hhdr = horizontalHeader();
    hhdr->setSectionsMovable(false);
    hhdr->setStretchLastSection(true);
    hhdr->setCascadingSectionResizes(true);

    // Make vertical header non-movable and ensure that each line has the
    // same height, based on the font metrics.
    auto vhdr = verticalHeader();
    vhdr->setSectionsMovable(false);
    vhdr->setDefaultSectionSize(fontMetrics().height());
    vhdr->setSectionResizeMode(QHeaderView::Fixed);

    // Create an empty dictionary.
    reset_dictionary();
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
    if (rowCount() == 0) {
        insertRow(0);
        return;
    }

    insertRow(rowCount() - 1 + 1);
}

void SmythDictionary::delete_columns(bool) {
    DeleteSelectedColumns();
}

void SmythDictionary::delete_rows(bool) {
    DeleteSelectedRows();
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
    if (
        state() == NoState and
        event->key() == Qt::Key_Delete and
        not selectedRanges().empty()
    ) {
        DeleteSelectedRows();
        return;
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
    for (auto [index, e] : arr | vws::enumerate) {
        auto text = Try(Serialiser<QString>::Deserialise(e));
        if (text.isEmpty()) continue;
        dict->setHorizontalHeaderItem(int(index), new QTableWidgetItem{text});
    }

    dict->setColumnCount(int(arr.size()));
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

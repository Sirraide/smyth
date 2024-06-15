#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <Smyth/JSON.hh>
#include <UI/App.hh>
#include <UI/SmythDictionary.hh>

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
SmythDictionary::SmythDictionary(QWidget* parent) : QTableWidget(parent) {
    setAlternatingRowColors(true);

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

void SmythDictionary::keyPressEvent(QKeyEvent* event) {
    if (HandleZoomEvent(event)) return;

    // Prompt user to delete selected row.
    if (
        state() == NoState and
        (event->key() == Qt::Key_Delete or event->key() == Qt::Key_Backspace) and
        not selectedRanges().empty()
    ) {
        // Figure out what rows we’re supposed to delete.
        std::vector<int> rows;
        for (auto rng : selectedRanges())
            for (int row = rng.topRow(); row <= rng.bottomRow(); ++row)
                rows.push_back(row);

        // Prompt the user to delete the rows.
        auto res = QMessageBox::question(
            this,
            "Deleting Rows",
            QString::fromStdString(std::format("Are you sure you want to delete {} row(s)?", rows.size())),
            QMessageBox::Yes | QMessageBox::No
        );

        // Dew it.
        if (res == QMessageBox::Yes) {
            rgs::sort(rows, std::greater<int>{});
            for (auto row : rows) removeRow(row);
        }

        // If we end up with no rows as a result, insert a new one.
        if (rowCount() == 0) add_row();

        // In any case, don’t process this key press any further.
        return;
    }

    QTableWidget::keyPressEvent(event);
}

void SmythDictionary::persist(PersistentStore& root_store, std::string_view key) {
    PersistentStore& store = App::CreateStore(std::string{key}, root_store);
    store.register_entry("columns", std::make_unique<PersistColumns>(this));
    store.register_entry("contents", std::make_unique<PersistContents>(this));
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

    // Set the column count.
    dict->setColumnCount(int(col_count));
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

#include <QHeaderView>
#include <UI/App.hh>
#include <UI/SmythDictionary.hh>
#include <Smyth/JSON.hh>

using namespace smyth;
using namespace smyth::json_utils;
using namespace smyth::ui;
using smyth::detail::Serialiser;

namespace {
struct PersistDictionaryContents : smyth::detail::PersistentBase {
    SmythDictionary* dict;
    PersistDictionaryContents(SmythDictionary* dict) : dict{dict} {}

    auto load(const json& j) -> Result<> override;
    void restore() override;
    auto save() const -> Result<json> override;
};
} // namespace

SmythDictionary::SmythDictionary(QWidget* parent) : QTableWidget(parent) {
    setAlternatingRowColors(true);

    App::The().serif_font.subscribe(this, &SmythDictionary::setFont);

    auto hhdr = horizontalHeader();
    hhdr->setSectionsMovable(false);
    hhdr->setStretchLastSection(true);
    hhdr->setCascadingSectionResizes(true);

    auto vhdr = verticalHeader();
    vhdr->setSectionsMovable(false);
    vhdr->setDefaultSectionSize(fontMetrics().height());
    vhdr->setSectionResizeMode(QHeaderView::Fixed);

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

    // Do not insert a new row if the last row is completely empty.
    auto last = rowCount() - 1;
    for (int col = 0; col < columnCount(); ++col) {
        auto it = item(last, col);
        if (it and not it->text().isEmpty()) {
            insertRow(last + 1);
            return;
        }
    }
}

void SmythDictionary::persist(PersistentStore& root_store, std::string_view key) {
    PersistentStore& store = App::CreateStore(std::string{key}, root_store);
    store.register_entry("contents", std::make_unique<PersistDictionaryContents>(this));
}

void SmythDictionary::reset_dictionary() {
    clear();
    add_row();
    setRowCount(1);
    setColumnCount(2);
}

auto PersistDictionaryContents::load(const json& j) -> Result<> {
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

void PersistDictionaryContents::restore() {
    dict->reset_dictionary();
}

auto PersistDictionaryContents::save() const -> Result<json> {
    json::array_t rows;
    for (int row = 0; row < dict->rowCount(); ++row) {
        json::array_t cols;
        for (int col = 0; col < dict->columnCount(); ++col) {
            auto it = dict->item(row, col);
            if (it) cols.push_back(Serialiser<QString>::Serialise(it->text()));
        }
        rows.push_back(std::move(cols));
    }
    return std::move(rows);
}

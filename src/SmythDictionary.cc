#include <QHeaderView>
#include <UI/App.hh>
#include <UI/SmythDictionary.hh>

using namespace smyth::ui;

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

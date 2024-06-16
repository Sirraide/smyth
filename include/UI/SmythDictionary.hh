#ifndef SMYTH_UI_DICTIONARY_HH
#define SMYTH_UI_DICTIONARY_HH

#include "Mixins.hh"

#include <QHeaderView>
#include <QTableWidget>
#include <UI/CSVExportImportDialog.hh>
#include <UI/Mixins.hh>

// General concept:
//
// Separate saving logic so only rows that have changed are saved on every
// operation that changes a row.
//
// CTRL+C when an entry is selected to copy the word.
//
// Double-click an entry to *open* it. This shows the entry in a dialog
//     that contains a more detailed view of it (including e.g. all senses,
//     examples, etc.)
//
// Right-click an entry to open a menu that lets you:
//     - Delete an entry
//     - Edit an entry (this brings up another dialog)
//
// Button to add a new entry. This opens a dialog that, unlike the other
//     dialogs described above, is *NOT* modal, so people can still use
//     Smyth while adding an entry.
//
// Search bar at the top that lets you filter entries (by any column; use
//     a select menu for that); search uses NFKD on the search input; the
//     search also applies NFKD to the field being searched.
//
// Allow hiding row numbers.
//
// Sort entries by NFKD of the headword.
//
// Export to LaTeX.

namespace smyth::ui {
class SmythDictionary;
}

namespace smyth::ui::detail {
class ColumnHeaders;
}

class smyth::ui::detail::ColumnHeaders : public QHeaderView {
    Q_OBJECT

public:
    ColumnHeaders(QWidget* parent = nullptr)
        : QHeaderView(Qt::Horizontal, parent) {
        setSectionsClickable(true);
        setSortIndicatorClearable(true);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

class smyth::ui::SmythDictionary final : public QTableWidget
    , mixins::Zoom
    , mixins::PromptUser {
    Q_OBJECT

    friend Zoom;

    using This = SmythDictionary;

    CSVExportImportDialog import_dialog;
    CSVExportImportDialog export_dialog;
    QMenu* context_menu;

public:
    SmythDictionary(QWidget* parent = nullptr);
    ~SmythDictionary() override;

    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void persist(PersistentStore& store);
    void reset_dictionary();

    void debug();

    void setFont(const QFont& font) {
        QTableWidget::setFont(font);
        verticalHeader()->setDefaultSectionSize(fontMetrics().height());
    }

    void wheelEvent(QWheelEvent* event) override {
        if (HandleZoomEvent(event)) return;
        QTableWidget::wheelEvent(event);
    }

public slots:
    void add_column();
    void add_row();
    void delete_columns(bool);
    void delete_rows(bool);
    void import();
    void import_and_replace();
    void export_dictionary();

private:
    void DeleteSelectedColumns();
    void DeleteSelectedRows();
    auto ImportCSV(bool replace) -> Result<>;
    auto ExportCSV() -> Result<>;
};

#endif // SMYTH_UI_DICTIONARY_HH

#ifndef SMYTH_UI_SMYTH_NOTES_LIST_HH
#define SMYTH_UI_SMYTH_NOTES_LIST_HH

#include <QListWidget>
#include <UI/Mixins.hh>

namespace smyth::ui {
class SmythNotesList;
}

namespace smyth::ui::detail {
class PersistNotesList;
}

class smyth::ui::SmythNotesList final : public QListWidget
    , mixins::Zoom
    , mixins::PromptUser {
    Q_OBJECT

    class Item;
    friend Zoom;
    friend detail::PersistNotesList;

    QMenu* context_menu;
    bool ignore_current_changed = false;

public:
    SmythNotesList(QWidget* parent = nullptr);

    void contextMenuEvent(QContextMenuEvent* event) override;
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
    auto item(int index) const -> Item*;
    void init();
    void keyPressEvent(QKeyEvent* event) override;
    void persist(PersistentStore& notes_store);

    void wheelEvent(QWheelEvent* event) override {
        if (HandleZoomEvent(event)) return;
        QListWidget::wheelEvent(event);
    }

public slots:
    void delete_file();
    void export_file();
    void new_file();
    void rename_file();

private:
    auto ExportFile() -> Result<>;
    auto TextBox() -> SmythPlainTextEdit*;
    void SetCurrentItem(Item* it);
};

#endif // SMYTH_UI_SMYTH_NOTES_LIST_HH

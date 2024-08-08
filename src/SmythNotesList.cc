#include <base/Defer.hh>
#include <QMenu>
#include <UI/App.hh>
#include <UI/MainWindow.hh>
#include <UI/SmythNotesList.hh>
#include <UI/SmythRichTextEdit.hh>

import smyth.json;
import smyth.persistent;

using namespace smyth;
using namespace smyth::ui;
using namespace smyth::json_utils;

class SmythNotesList::Item : public QListWidgetItem {
public:
    QString file_contents;
    Item(const QString& text, QString contents = "")
        : QListWidgetItem(text),
          file_contents(std::move(contents)) {
        setFlags(flags() | Qt::ItemIsEditable);
    }
};

class ui::detail::PersistNotesList : public smyth::detail::PersistentBase {
    SmythNotesList* list;

public:
    PersistNotesList(SmythNotesList* list) : list(list) {}

    auto load(const json& j) -> Result<> override;
    void restore() override;
    auto save() const -> Result<json> override;
};

SmythNotesList::SmythNotesList(QWidget* parent) : QListWidget(parent) {
    setSelectionBehavior(SelectRows);

    {
        context_menu = new QMenu(this);
        auto new_file = context_menu->addAction("New");
        auto delete_file = context_menu->addAction("Delete");
        auto rename_file = context_menu->addAction("Rename");
        connect(new_file, &QAction::triggered, this, &SmythNotesList::new_file);
        connect(delete_file, &QAction::triggered, this, &SmythNotesList::delete_file);
        connect(rename_file, &QAction::triggered, this, &SmythNotesList::rename_file);
    }
}

auto SmythNotesList::TextBox() -> SmythRichTextEdit* {
    return App::MainWindow()->notes_tab_text_box();
}

void SmythNotesList::SetCurrentItem(Item* it) {
    Assert(it != nullptr);
    tempset ignore_current_changed = true;
    setCurrentItem(it);
    TextBox()->setPlainText(it->file_contents);
}

void SmythNotesList::contextMenuEvent(QContextMenuEvent* event) {
    context_menu->exec(event->globalPos());
}

void SmythNotesList::currentChanged(
    const QModelIndex& selected,
    const QModelIndex& deselected
) {
    if (ignore_current_changed) return;
    auto text_box = TextBox();

    if (deselected.internalPointer()) {
        auto old_item = static_cast<Item*>(deselected.internalPointer());
        old_item->file_contents = text_box->toPlainText();
    }

    SetCurrentItem(
        selected.internalPointer()
            ? static_cast<Item*>(selected.internalPointer())
            : item(0)
    );
}

void SmythNotesList::delete_file() {
    tempset ignore_current_changed = true;
    auto selected = selectedItems();
    if (selected.isEmpty()) return;
    if (not Prompt("Delete file", "Are you sure you want to delete {} file(s)?", selected.size())) return;
    for (auto item : selected) delete item;
    if (count() == 0) new_file();
    SetCurrentItem(item(0));
}

void SmythNotesList::init() {
    // Ensure that we have at least one file.
    tempset ignore_current_changed = true;
    new_file();
}

auto SmythNotesList::item(int index) const -> Item* {
    return static_cast<Item*>(QListWidget::item(index));
}

void SmythNotesList::keyPressEvent(QKeyEvent* event) {
    if (HandleZoomEvent(event)) return;
    if (event->key() == Qt::Key_Delete) {
        delete_file();
        return;
    }

    QListWidget::keyPressEvent(event);
}

void SmythNotesList::new_file() {
    addItem(new Item("New file"));
    if (count() == 1) SetCurrentItem(item(0));
}

void SmythNotesList::rename_file() {
    auto items = selectedItems();
    if (items.isEmpty()) return;
    editItem(items.front());
}

/// ====================================================================
///  Persistence
/// ====================================================================
void SmythNotesList::persist(void* _notes_store) {
    auto& notes_store = *static_cast<PersistentStore*>(_notes_store);
    notes_store.register_entry("files", std::make_unique<detail::PersistNotesList>(this));
}

using namespace smyth::ui::detail;
auto PersistNotesList::load(const json& j) -> Result<> {
    // Make sure we ignore changes in the current item here.
    tempset list->ignore_current_changed = true;

    // Remove all old items.
    list->clear();

    // Load all items from the save file.
    const json::array_t& arr = Try(Get<json::array_t>(j));
    for (const json& item : arr) {
        if (not item.contains("name")) return Error("Missing 'name' field in note");
        if (not item.contains("contents")) return Error("Missing 'contents' field in note");
        auto name = Try(smyth::detail::Serialiser<QString>::Deserialise(item["name"]));
        auto contents = Try(smyth::detail::Serialiser<QString>::Deserialise(item["contents"]));
        list->addItem(new SmythNotesList::Item(name, contents));
    }

    // Ensure there is at least one item and select the first one.
    if (list->count() == 0) list->new_file();
    list->SetCurrentItem(list->item(0));
    return {};
}

void PersistNotesList::restore() {
    tempset list->ignore_current_changed = true;
    list->clear();
    list->new_file();
}

auto PersistNotesList::save() const -> Result<json> {
    // Update current item.
    static_cast<SmythNotesList::Item*>(list->currentItem())->file_contents =
        list->TextBox()->toPlainText();

    // Save them all.
    json::array_t arr;
    for (int i = 0; i < list->count(); ++i) {
        auto item = list->item(i);
        json j;
        j["name"] = smyth::detail::Serialiser<QString>::Serialise(item->text());
        j["contents"] = smyth::detail::Serialiser<QString>::Serialise(item->file_contents);
        arr.push_back(std::move(j));
    }
    return std::move(arr);
}

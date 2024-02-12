#ifndef SMYTH_UI_VFSTREEITEM_HH
#define SMYTH_UI_VFSTREEITEM_HH

#include <memory>
#include <QAbstractItemModel>
#include <QStandardItem>
#include <Smyth/VFS.hh>

namespace smyth::ui {
class VFSTreeItem : public QStandardItem {
public:
    vfs::FileID file_id;

    VFSTreeItem(vfs::FileID file_id, QString file_name)
        : QStandardItem(file_name), file_id(file_id) {
        setCheckable(false);
        setEditable(true);
    }

    int type() const override { return QStandardItem::UserType + 1; }

    [[nodiscard]] constexpr auto column_count() const -> int { return 1; }
    [[nodiscard]] auto row() const -> int;
};

class VFSTreeModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    std::unique_ptr<VFSTreeItem> root;

    VFSTreeModel(QObject* parent)
        : QAbstractItemModel(parent),
          root(std::make_unique<VFSTreeItem>(vfs::FileID::Root(), "/")) {}

    /// Get the item at an index.
    [[nodiscard]] auto item(const QModelIndex& index) const -> VFSTreeItem*;

    /// Insert an item.
    void insert(VFSTreeItem* parent, VFSTreeItem* child);

    auto columnCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto index(int row, int column, const QModelIndex& parent) const -> QModelIndex override;
    auto parent(const QModelIndex& child) const -> QModelIndex override;
    auto rowCount(const QModelIndex& parent) const -> int override;
};
} // namespace smyth::ui

#endif // SMYTH_UI_VFSTREEITEM_HH

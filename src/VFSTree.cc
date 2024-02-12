#include <UI/VFSTree.hh>

using namespace smyth;
using namespace smyth::ui;

auto VFSTreeModel::columnCount(const QModelIndex& parent) const -> int {
    if (not parent.isValid()) return root->column_count();
    return static_cast<VFSTreeItem*>(parent.internalPointer())->column_count();
}

auto VFSTreeModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (not index.isValid() or role != Qt::DisplayRole) return {};
    return static_cast<VFSTreeItem*>(index.internalPointer())->data(role);
}

auto VFSTreeModel::index(int row, int column, const QModelIndex& parent) const -> QModelIndex {
    if (not hasIndex(row, column, parent)) return {};
    auto parent_item //
        = parent.isValid()
            ? static_cast<VFSTreeItem*>(parent.internalPointer())
            : root.get();

    auto child = parent_item->child(row);
    return child ? createIndex(row, column, child) : QModelIndex{};
}

void VFSTreeModel::insert(VFSTreeItem* parent, VFSTreeItem* child) {
    beginInsertRows(child->index(), child->rowCount(), child->rowCount());
    parent->appendRow(child);
    endInsertRows();
    parent->sortChildren(0, Qt::SortOrder::AscendingOrder);
}

auto VFSTreeModel::item(const QModelIndex& index) const -> VFSTreeItem* {
    if (not index.isValid()) return root.get();
    return static_cast<VFSTreeItem*>(index.internalPointer());
}

auto VFSTreeModel::parent(const QModelIndex& child) const -> QModelIndex {
    if (not child.isValid()) return {};
    auto child_item = static_cast<VFSTreeItem*>(child.internalPointer());
    auto parent_item = child_item->parent();
    if (parent_item == root.get()) return {};
    return createIndex(parent_item->row(), 0, parent_item);
}

auto VFSTreeModel::rowCount(const QModelIndex& parent) const -> int {
    if (parent.column() > 0) return 0;
    if (not parent.isValid()) return root->rowCount();
    return static_cast<VFSTreeItem*>(parent.internalPointer())->rowCount();
}

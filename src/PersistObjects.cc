#include <UI/PersistObjects.hh>
#include <UI/App.hh>

void smyth::ui::PersistCBox(std::string key, QComboBox* cbox) {
    App::The().persist<&QComboBox::currentIndex, &QComboBox::setCurrentIndex>(
        std::move(key),
        cbox
    );
}

void smyth::ui::PersistChBox(std::string key, QCheckBox* cbox) {
    App::The().persist<&QCheckBox::checkState, &QCheckBox::setCheckState>(
        std::move(key),
        cbox
    );
}

/// Like PersistCBox, but the entries are generated dynamically.
void smyth::ui::PersistDynCBox(std::string key, QComboBox* cbox) {
    App::The().persist<&QComboBox::currentText, [](QComboBox* cbox, QString str) {
        // If the item does not exists yet, add it.
        if (cbox->findText(str) == -1) cbox->addItem(str);
        cbox->setCurrentText(str);
    }>(std::move(key), cbox);
}

/// Splitters may crash if we supply a value that is larger than the total width.
void smyth::ui::PersistSplitter(std::string key, QSplitter* splitter) {
    App::The().persist<&QSplitter::sizes, [](QSplitter* w, QList<int> sz) {
        // If the sum is greater than the width, ignore.
        if (std::accumulate(sz.begin(), sz.end(), 0) > w->width()) {
            Debug("Ignoring invalid saved splitter sizes: {} vs total width {}", sz, w->width());
            return;
        }

        w->setSizes(sz);
    }>(std::move(key), splitter);
}

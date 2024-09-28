#ifndef SMYTH_UI_SMYTHRICHTEXTEDIT_HH
#define SMYTH_UI_SMYTHRICHTEXTEDIT_HH

#include <QTextEdit>
#include <UI/Smyth.hh>
#include <UI/Mixins.hh>

namespace smyth::ui {
class SmythRichTextEdit;
} // namespace smyth::ui

class smyth::ui::SmythRichTextEdit final : public QTextEdit
    , mixins::Zoom {
    Q_OBJECT

    friend Zoom;

    using This = SmythRichTextEdit;

public:
    SmythRichTextEdit(QWidget* parent = nullptr)
        : QTextEdit(parent) {}

    void persist(PersistentStore& store, std::string_view key, bool include_text = true) {
        if (include_text) Persist<&This::toHtml, &This::setHtml>(
            store,
            std::format("{}.text", key),
            this
        );
    }

    void wheelEvent(QWheelEvent* event) override {
        if (HandleZoomEvent(event)) return;
        QTextEdit::wheelEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (HandleZoomEvent(event)) return;

        // Tabs suck.
        if (event->key() == Qt::Key_Tab) {
            insertPlainText("    ");
            event->accept();
            return;
        }

        QTextEdit::keyPressEvent(event);
    }
};

#endif // SMYTH_UI_SMYTHRICHTEXTEDIT_HH

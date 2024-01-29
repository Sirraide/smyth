#ifndef SMYTH_UI_SMYTHRICHTEXTEDIT_HH
#define SMYTH_UI_SMYTHRICHTEXTEDIT_HH

#include <QTextEdit>
#include <UI/App.hh>
#include <UI/Common.hh>

namespace smyth::ui {
class SmythRichTextEdit final : public QTextEdit {
    Q_OBJECT

    using This = SmythRichTextEdit;

public:
    SmythRichTextEdit(QWidget* parent = nullptr)
        : QTextEdit(parent) {}

    void persist(App& app, std::string_view key, bool include_text = true) {
        if (include_text) app.persist<&This::toHtml, &This::setHtml>(
            std::format("{}.text", key),
            this
        );

        app.persist<&This::font, &This::setFont>(
            std::format("{}.font", key),
            this
        );
    }

    void wheelEvent(QWheelEvent* event) override {
        if (common::HandleZoomEvent(this, event)) return;
        QTextEdit::wheelEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (common::HandleZoomEvent(this, event)) return;
        if (event->key() == Qt::Key_Tab) {
            insertPlainText("    ");
            event->accept();
            return;
        }

        QTextEdit::keyPressEvent(event);
    }
};
} // namespace smyth::ui

#endif // SMYTH_UI_SMYTHRICHTEXTEDIT_HH

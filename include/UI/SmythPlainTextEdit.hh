#ifndef SMYTH_UI_SMYTHPLAINTEXTEDIT_HH
#define SMYTH_UI_SMYTHPLAINTEXTEDIT_HH

#include <QPlainTextEdit>
#include <UI/App.hh>
#include <UI/Common.hh>

namespace smyth::ui {
class SmythPlainTextEdit final : public QPlainTextEdit {
    Q_OBJECT

    using This = SmythPlainTextEdit;

public:
    SmythPlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit(parent) {}

    void persist(App& app, std::string_view key) {
        app.persist<&This::toPlainText, &This::setPlainText>(
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
        QPlainTextEdit::wheelEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (common::HandleZoomEvent(this, event)) return;
        if (event->key() == Qt::Key_Tab) {
            insertPlainText("    ");
            event->accept();
            return;
        }

        QPlainTextEdit::keyPressEvent(event);
    }
};
} // namespace smyth::ui

#endif // SMYTH_UI_SMYTHPLAINTEXTEDIT_HH

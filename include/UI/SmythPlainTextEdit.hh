#ifndef SMYTH_UI_SMYTHPLAINTEXTEDIT_HH
#define SMYTH_UI_SMYTHPLAINTEXTEDIT_HH

#include <QPlainTextEdit>
#include <UI/App.hh>
#include <UI/Mixins.hh>

namespace smyth::ui {
class SmythPlainTextEdit;
} // namespace smyth::ui

class smyth::ui::SmythPlainTextEdit final : public QPlainTextEdit
    , mixins::Zoom {
    Q_OBJECT

    friend Zoom;

    using This = SmythPlainTextEdit;

public:
    SmythPlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit(parent) {}

    void persist(PersistentStore& store, std::string_view key) {
        Persist<&This::toPlainText, &This::setPlainText>(
            store,
            std::format("{}.text", key),
            this
        );

        Persist<&This::font, &This::setFont>(
            store,
            std::format("{}.font", key),
            this
        );
    }

    void wheelEvent(QWheelEvent* event) override {
        if (HandleZoomEvent(event)) return;
        QPlainTextEdit::wheelEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (HandleZoomEvent(event)) return;

        // Tabs suck.
        if (event->key() == Qt::Key_Tab) {
            insertPlainText("    ");
            event->accept();
            return;
        }

        QPlainTextEdit::keyPressEvent(event);
    }
};

#endif // SMYTH_UI_SMYTHPLAINTEXTEDIT_HH

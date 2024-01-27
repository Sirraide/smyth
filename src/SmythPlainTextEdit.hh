#ifndef SMYTH_SMYTHPLAINTEXTEDIT_HH
#define SMYTH_SMYTHPLAINTEXTEDIT_HH

#include <QPlainTextEdit>

class SmythPlainTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    SmythPlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit(parent) {
        setTabChangesFocus(true);
    }

    void wheelEvent(QWheelEvent* event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->angleDelta().y() > 0) ZoomIn();
            else ZoomOut();
            event->accept();
        } else {
            QPlainTextEdit::wheelEvent(event);
        }
    }

    void ZoomIn() {
        setFont(QFont(font().family(), font().pointSize() + 1));
    }

    void ZoomOut() {
        setFont(QFont(font().family(), font().pointSize() - 1));
    }

    void ZoomReset() {
        setFont(QFont(font().family(), 10));
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            switch (event->key()) {
                case Qt::Key_Plus:
                    ZoomIn();
                    event->accept();
                    return;

                case Qt::Key_Minus:
                    ZoomOut();
                    event->accept();
                    return;

                case Qt::Key_0:
                    ZoomReset();
                    event->accept();
                    return;
            }
        }

        QPlainTextEdit::keyPressEvent(event);
    }
};

#endif // SMYTH_SMYTHPLAINTEXTEDIT_HH

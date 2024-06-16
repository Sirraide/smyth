#ifndef SMYTH_UI_COMMON_HH
#define SMYTH_UI_COMMON_HH

#include <QFont>
#include <QMessageBox>
#include <QWheelEvent>

namespace smyth::ui::mixins {
struct PromptUser;
struct Zoom;
} // namespace smyth::ui::mixins

/// Prompt the user whether they actually want to perform an action.
struct smyth::ui::mixins::PromptUser {
    /// Returns whether the user accepted the prompt.
    template <typename... Args>
    bool Prompt(this auto&& self, QString title, std::format_string<Args...> fmt, Args&&... args) {
        auto res = QMessageBox::question(
            std::addressof(self),
            title,
            QString::fromStdString(std::format(fmt, std::forward<Args>(args)...)),
            QMessageBox::Yes | QMessageBox::No
        );
        return res == QMessageBox::Yes;
    }
};

/// Zoom mixin.
///
/// This enables the user to resize the font of a widget
/// using CTRL + Mouse Wheel or CTRL + Plus/Minus.
struct smyth::ui::mixins::Zoom {
    bool HandleZoomEvent(this auto&& w, QWheelEvent* event) {
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->angleDelta().y() > 0) w.ZoomIn();
            else w.ZoomOut();
            event->accept();
            return true;
        }
        return false;
    }

    bool HandleZoomEvent(this auto&& w, QKeyEvent* event) {
        if (event->modifiers() & Qt::ControlModifier) {
            switch (event->key()) {
                case Qt::Key_Plus:
                    w.ZoomIn();
                    event->accept();
                    return true;

                case Qt::Key_Minus:
                    w.ZoomOut();
                    event->accept();
                    return true;

                case Qt::Key_0:
                    w.ZoomReset();
                    event->accept();
                    return true;
            }
        }
        return false;
    }

private:
    void ZoomIn(this auto&& w) {
        w.setFont(QFont(w.font().family(), w.font().pointSize() + 1));
    }

    void ZoomOut(this auto&& w) {
        w.setFont(QFont(w.font().family(), w.font().pointSize() - 1));
    }

    void ZoomReset(this auto&& w) {
        w.setFont(QFont(w.font().family(), 10));
    }
};

#endif // SMYTH_UI_COMMON_HH

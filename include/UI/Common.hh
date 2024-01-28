#ifndef SMYTH_UI_COMMON_HH
#define SMYTH_UI_COMMON_HH

#include <QFont>
#include <QWheelEvent>

namespace smyth::ui::common {
template <typename Widget>
void ZoomIn(Widget* w) {
    w->setFont(QFont(w->font().family(), w->font().pointSize() + 1));
}

template <typename Widget>
void ZoomOut(Widget* w) {
    w->setFont(QFont(w->font().family(), w->font().pointSize() - 1));
}

template <typename Widget>
void ZoomReset(Widget* w) {
    w->setFont(QFont(w->font().family(), 10));
}

template <typename Widget>
bool HandleZoomEvent(Widget* w, QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) ZoomIn(w);
        else ZoomOut(w);
        event->accept();
        return true;
    }
    return false;
}

template <typename Widget>
bool HandleZoomEvent(Widget* w, QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_Plus:
                ZoomIn(w);
                event->accept();
                return true;

            case Qt::Key_Minus:
                ZoomOut(w);
                event->accept();
                return true;

            case Qt::Key_0:
                ZoomReset(w);
                event->accept();
                return true;
        }
    }
    return false;
}

} // namespace smyth::ui::common

#endif // SMYTH_UI_COMMON_HH

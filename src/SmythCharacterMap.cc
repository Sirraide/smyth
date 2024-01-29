#include <QApplication>
#include <QPainter>
#include <UI/MainWindow.hh>
#include <UI/SmythCharacterMap.hh>

auto smyth::ui::SmythCharacterMap::Rows() const -> int {
    /// Ensure we always have at least one column.
    if (cols == 0) return 1;
    return int(chars.size()) / cols + (int(chars.size()) % cols ? 1 : 0);
}

auto smyth::ui::SmythCharacterMap::MinHeight() const -> int {
    /// Add one to account for the bottom line of the grid.
    return Rows() * square_height + 1;
}

void smyth::ui::SmythCharacterMap::UpdateChars() {
    QFontMetrics m{font()};
    chars.clear();
    for (char32_t i = FirstPrintChar; i < last_codepoint; i++)
        if (m.inFontUcs4(i))
            chars.push_back(QString::fromUcs4(&i, 1));

    /// Determine the size of each square relative to the font size.
    square_height = std::max(m.height(), m.maxWidth()) + 10;
    UpdateSize();
}

void smyth::ui::SmythCharacterMap::UpdateSize() {
    QFontMetrics m{font()};

    /// Determine the number of columns that we can draw and adjust our dimensions.
    cols = parentWidget()->width() / square_height;
    adjustSize();
    setMinimumHeight(MinHeight());

    /// Recalculate the square size to include the remaining space that we
    /// couldn’t fit another square into.
    const auto space_left = (width() - (square_height * cols));
    square_width = std::max(m.height(), m.maxWidth()) + 10 + space_left / cols;
    setMinimumWidth(square_width);
}

void smyth::ui::SmythCharacterMap::mousePressEvent(QMouseEvent* event) {
    auto pos = mapFromGlobal(event->globalPosition());
    auto idx = int(pos.y() / square_height) * cols + int(pos.x() / square_width);
    if (idx >= 0 and idx < int(chars.size())) {
        selected_idx = idx;
        emit selected(chars[usz(idx)].toUcs4().front());
        update();
    }
}

/// Code below adapted from https://doc.qt.io/qt-5/qtwidgets-widgets-charactermap-example.html.
void smyth::ui::SmythCharacterMap::paintEvent(QPaintEvent* event) {
    QFontMetrics m{font()};
    QPainter painter{this};
    QRect redraw = event->rect();
    painter.setFont(font());

    /// Determine the area that we need to redraw.
    ///
    /// Take care not to draw more characters than we have.
    const int max_char = int(chars.size());
    const int max_rows = max_char / cols + (max_char % cols ? 1 : 0);
    const int row_begin = redraw.top() / square_height;
    const int row_end = std::min(max_rows, redraw.bottom() / square_height + 1);
    const int col_begin = redraw.left() / square_height;

    /// Draw grid.
    QPen grid{QApplication::palette().accent().color()};
    painter.setPen(grid);
    for (int r = row_begin; r < row_end; ++r) {
        for (int c = col_begin; c < cols; ++c) {
            /// Last row may be incomplete.
            if (usz(r * cols + c) >= chars.size()) goto draw_chars;
            int x = c * square_width;
            int y = r * square_height;
            painter.drawRect(x, y, square_width, square_height);

            /// Draw a background if this is the selected character.
            if (int(r * cols + c) == selected_idx) {
                QBrush brush{QApplication::palette().accent().color()};
                painter.fillRect(x + 1, y + 1, square_width - 1, square_height - 1, brush);
            }
        }
    }

draw_chars:
    /// Draw characters.
    const int default_char_height = m.descent() + m.ascent();
    QPen pen{QApplication::palette().text().color()};
    painter.setPen(pen);
    for (int r = row_begin; r < row_end; ++r) {
        for (int c = col_begin; c < cols; ++c) {
            /// Last row may be incomplete.
            auto k_idx = usz(r * cols + c);
            if (k_idx >= chars.size()) return;

            /// Draw the character.
            auto& s = chars[k_idx];
            int x = c * square_width;
            int y = r * square_height;
            painter.setClipRect(x, y, square_width, square_height);
            painter.drawText(
                x + (square_width / 2) - m.horizontalAdvance(s) / 2,
                y + square_height - (square_height - default_char_height) / 2 - m.descent(),
                s
            );
        }
    }
}

void smyth::ui::SmythCharacterMap::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    UpdateSize();
}

void smyth::ui::SmythCharacterMap::setFont(const QFont& font) {
    QFont f(font);
    f.setStyleStrategy(QFont::NoFontMerging);
    QWidget::setFont(f);
    UpdateChars();
}

auto smyth::ui::SmythCharacterMap::sizeHint() const -> QSize {
    /// Add one to account for the right line of the grid.
    QSize s {parentWidget()->width(), MinHeight()};
    return s;
}

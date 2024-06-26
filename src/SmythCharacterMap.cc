#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <set>
#include <UI/MainWindow.hh>
#include <UI/SmythCharacterMap.hh>

auto smyth::ui::SmythCharacterMap::ClickToIndex(QMouseEvent* event) -> std::optional<int> {
    auto pos = mapFromGlobal(event->globalPosition());
    auto idx = int(pos.y() / square_height) * cols + int(pos.x() / square_width);
    if (idx >= 0 and idx < int(DisplayedCodepoints().size())) return idx;
    return std::nullopt;
}

void smyth::ui::SmythCharacterMap::CollectChars(
    const QFontMetrics& m,
    c32 from,
    c32 to,
    std::vector<QString>& chars,
    std::vector<c32>& codepoints
) {
    chars.clear();
    codepoints.clear();
    for (c32 i = from; i <= to; i++) {
        if (m.inFontUcs4(i)) {
            codepoints.push_back(i);
            chars.push_back(QString::fromUcs4(&i.value, 1));
        }
    }
}

auto smyth::ui::SmythCharacterMap::DisplayedChars() const -> const std::vector<QString>& {
    return last_query.isEmpty() ? all_chars : matched_chars;
}

auto smyth::ui::SmythCharacterMap::DisplayedCodepoints() const -> const std::vector<c32>& {
    return last_query.isEmpty() ? all_codepoints : matched_codepoints;
}

auto smyth::ui::SmythCharacterMap::MinHeight() const -> int {
    // Add one to account for the bottom line of the grid.
    return Rows() * square_height + 1;
}

auto smyth::ui::SmythCharacterMap::Rows() const -> int {
    // Ensure we always have at least one column.
    if (cols == 0) return 1;
    return int(DisplayedChars().size()) / cols + (int(DisplayedChars().size()) % cols ? 1 : 0);
}

/// <query>          ::= <codepoint> | <range> | <name> | <literals>
///
/// <codepoint>      ::= [ "U" | "u" ] [ "+" ] <hex-digits>
/// <literals>       ::= sequence containing any other character
/// <range>          ::= [ <codepoint> ] <to> [ <codepoint> ]
/// <name>           ::= <alnum-ws> <alnum-ws> { <alnum-ws> }
///                    | <q-sequence>
///
/// <alnum-ws>       ::= any alphanumeric ASCII character or whitespace
/// <ascii>          ::= any ASCII character
/// <to>             ::= "-" | ":" | ".." | "..."
/// <q-sequence>     ::= "'" { <any-character> } "'"
///                    | '"' { <any-character> } '"'
///                    | "`" { <any-character> } "`"
///
/// Leading and trailing whitespace is ignored for any rule except <literals>.
auto smyth::ui::SmythCharacterMap::ParseQuery(QStringView query) -> Query {
    static const QRegularExpression codepoint_or_range{// clang-format off
        "^\\s*"
        "(?:"
            "(?:u|U)?"
            "\\+?"
            "([a-fA-F0-9]+)"
        ")?"
        "(?:"
            "([:-]|\\.\\.\\.?)"
            "(?:"
                "(?:u|U)?"
                "\\+?"
                "([a-fA-F0-9]+)"
            ")?"
        ")?"
        "\\s*$"
    }; // clang-format on

    static const QRegularExpression quoted_sequence{
        "^\\s*(['\"`])(.+)\\1\\s*$"
    };

    static const QRegularExpression name{
        "^\\s*([a-zA-Z0-9 ]+)\\s*$"
    };

    // Might as well get this out of the way early.
    if (query.isEmpty()) return std::monostate{};

    // Codepoint or range.
    if (
        query.startsWith(u'u') or
        query.startsWith(u'U') or
        query.startsWith(u'+') or
        query.startsWith(u'-') or
        query.startsWith(u':') or
        query.startsWith(u'.')
    ) {
        auto res = codepoint_or_range.match(query);
        Assert(res.isValid(), "Invalid regular expression: {}", codepoint_or_range.pattern());
        if (res.hasMatch()) {
            auto first_str = res.captured(1);
            auto second_str = res.captured(3);

            // We only have a delimiter. Treat this as literal text.
            c32 first = first_str.isNull() ? c32(0) : c32(first_str.toUInt(nullptr, 16));
            c32 second = second_str.isNull() ? c32(0) : c32(second_str.toUInt(nullptr, 16));
            if (first == 0 and second == 0) return Literal{res.captured(2)};

            // If we have a delimiter, then this is a range.
            if (not res.captured(2).isNull()) return Range{
                first == 0 ? FirstPrintChar : std::max(FirstPrintChar, first),
                second == 0 ? c32::max() : std::min<c32>(c32::max(), second)
            };
            Assert(first != 0, "Invalid codepoint");
            return Codepoint{first};
        }
    }

    // Quoted sequence.
    if (auto res = quoted_sequence.match(query); res.hasMatch())
        return Literal{res.captured(2)};

    // Name.
    if (auto res = name.match(query); res.hasMatch())
        return Name{res.captured(1)};

    // Literal.
    return Literal{query.toString()};
}

void smyth::ui::SmythCharacterMap::ProcessQuery(QStringView query) { // clang-format off
    using namespace utils;
    matched_chars.clear();
    matched_codepoints.clear();

    // Unique characters and sort them. There is no reason to preserve
    // the order since, if the user wants to copy them, they can just
    // copy them from the search bar.
    auto HandleLiteral = [&] (const QString& lit) {
        std::set<c32> chars;
        auto str = lit.toStdU32String();

        for (auto& c : str)
            if (rgs::binary_search(all_codepoints, c32(c)))
                chars.insert(c);

        for (auto c : chars) {
            matched_codepoints.push_back(c);
            matched_chars.push_back(QString::fromUcs4(&c.value, 1));
        }
    };

    Visit(ParseQuery(query), Overloaded{
        [](std::monostate) {},

        // If this codepoint exists in the current font, display only it.
        [&](Codepoint c) {
            if (rgs::binary_search(all_codepoints, c.value)) {
                matched_codepoints = {c.value};
                matched_chars = {QString::fromUcs4(&c.value.value, 1)};
            }
        },

        // Display all characters in this range that exist in the current font.
        [&](Range r) {
            auto first = rgs::lower_bound(all_codepoints, r.from);
            auto last = rgs::upper_bound(all_codepoints, r.to);
            if (first == last) return;
            if (first == all_codepoints.end()) first = all_codepoints.begin();

            // Do NOT use '!=' here as `last` may be < `it`!
            for (auto it = first; it < last; ++it) {
                matched_codepoints.push_back(*it);
                matched_chars.push_back(QString::fromUcs4(&it->value, 1));
            }
        },

        // Find characters whose name contains the given string(s).
        [&](const Name& n) {
            auto s = n.value.toUpper().toStdString();
            auto view = s | vws::split(' ') | vws::filter([](auto&& r) { return rgs::distance(r) != 0; });
            for (auto c : all_codepoints) {
                auto name = c.name();
                if (not name) continue;
                if (rgs::all_of(view, [&](auto&& r) { return name.value().contains(std::string_view{r}); })) {
                    matched_codepoints.push_back(c);
                    matched_chars.push_back(QString::fromUcs4(&c.value, 1));
                }
            }

            // If we couldn’t find any characters, just treat is as a literal.
            if (matched_codepoints.empty()) {
                HandleLiteral(n.value);
                return;
            }

            // Otherwise, display the chars we found.
            for (auto& c : matched_codepoints) {
                matched_chars.push_back(QString::fromUcs4(&c.value, 1));
            }
        },

        [&](const Literal& l) { HandleLiteral(l.value); },
    });

    // Also update the size of the widget since we most likely have
    // a different number of characters that we need to display.
    UpdateSize();
    update();
} // clang-format on

void smyth::ui::SmythCharacterMap::UpdateChars() {
    QFontMetrics m{font()};
    CollectChars(m, FirstPrintChar, c32::max(), all_chars, all_codepoints);

    // Determine the size of each square relative to the font size.
    square_height = std::max(m.height(), m.maxWidth()) + 10;
    UpdateSize();

    // Execute last query again.
    ProcessQuery(last_query);
}

void smyth::ui::SmythCharacterMap::UpdateSize() {
    QFontMetrics m{font()};

    // Determine the number of columns that we can draw and adjust our dimensions.
    cols = parentWidget()->width() / square_height;
    adjustSize();
    setMinimumHeight(MinHeight());

    // Recalculate the square size to include the remaining space that we
    // couldn’t fit another square into.
    const auto space_left = width() - square_height * cols;
    square_width = std::max(m.height(), m.maxWidth()) + 10 + space_left / cols;
    setMinimumWidth(square_width);
}

void smyth::ui::SmythCharacterMap::mouseDoubleClickEvent(QMouseEvent* event) {
    if (auto idx = ClickToIndex(event)) {
        QApplication::clipboard()->setText(DisplayedChars()[usz(*idx)]);
    }
}

void smyth::ui::SmythCharacterMap::mousePressEvent(QMouseEvent* event) {
    if (auto idx = ClickToIndex(event)) {
        selected_idx = *idx;
        emit selected(DisplayedCodepoints()[usz(*idx)]);
        update();
    }
}

/// Code below adapted from https://doc.qt.io/qt-5/qtwidgets-widgets-charactermap-example.html.
void smyth::ui::SmythCharacterMap::paintEvent(QPaintEvent* event) {
    QFontMetrics m{font()};
    QPainter painter{this};
    QRect redraw = event->rect();
    painter.setFont(font());

    // Determine the area that we need to redraw.
    //
    // Take care not to draw more characters than we have.
    const auto& chars = DisplayedChars();
    const int max_char = int(chars.size());
    const int max_rows = max_char / cols + (max_char % cols ? 1 : 0);
    const int row_begin = redraw.top() / square_height;
    const int row_end = std::min(max_rows, redraw.bottom() / square_height + 1);
    const int col_begin = redraw.left() / square_height;

    // Draw grid.
    QPen grid{QApplication::palette().light().color()};
    painter.setPen(grid);
    for (int r = row_begin; r < row_end; ++r) {
        for (int c = col_begin; c < cols; ++c) {
            // Last row may be incomplete.
            if (usz(r * cols + c) >= chars.size()) goto draw_chars;
            int x = c * square_width;
            int y = r * square_height;
            painter.drawRect(x, y, square_width, square_height);

            // Draw a background if this is the selected character.
            if (int(r * cols + c) == selected_idx) {
                QBrush brush{QApplication::palette().accent().color()};
                painter.fillRect(x + 1, y + 1, square_width - 1, square_height - 1, brush);
            }
        }
    }

draw_chars:
    // Draw characters.
    const int default_char_height = m.descent() + m.ascent();
    QPen pen{QApplication::palette().text().color()};
    painter.setPen(pen);
    for (int r = row_begin; r < row_end; ++r) {
        for (int c = col_begin; c < cols; ++c) {
            // Last row may be incomplete.
            auto k_idx = usz(r * cols + c);
            if (k_idx >= chars.size()) return;

            // Draw the character.
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

void smyth::ui::SmythCharacterMap::search(QString query) {
    ProcessQuery(query);
    last_query = std::move(query);
}

void smyth::ui::SmythCharacterMap::setFont(const QFont& font) {
    QFont f(font);
    f.setStyleStrategy(QFont::NoFontMerging);
    QWidget::setFont(f);
    UpdateChars();
}

auto smyth::ui::SmythCharacterMap::sizeHint() const -> QSize {
    // Add one to account for the right line of the grid.
    QSize s{parentWidget()->width(), MinHeight()};
    return s;
}

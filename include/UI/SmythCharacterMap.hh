#ifndef SMYTH_UI_SMYTHCHARACTERMAP_HH
#define SMYTH_UI_SMYTHCHARACTERMAP_HH

#include <QListView>
#include <QStringListModel>
#include <Smyth/Utils.hh>
#include <UI/App.hh>
#include <UI/Common.hh>

namespace smyth::ui {
class SmythCharacterMap final : public QWidget {
    Q_OBJECT

    /// First printable character.
    static constexpr char32_t FirstPrintChar = ' ';

    int cols{20};
    int square_height{40};
    int square_width{40};
    int selected_idx{-1};
    char32_t last_codepoint{0x10'FFFF};
    std::vector<QString> chars;

    /// TODO: Search bar:
    ///
    /// <query>     ::= <codepoint> | <range> | <name> | <literals>
    /// <codepoint> ::= [ "U" | "u" ] [ "+" ] <hex-digits>
    /// <name>      ::= sequence starting with any ASCII letter other than "U" or "u", or with any two ASCII letters.
    /// <literals>  ::= sequence containing any other character
    /// <range>     ::= [ <codepoint> ] <to> [ <codepoint> ]
    /// <to>        ::= "-" | "–" | ":"
    ///
    /// Leading and trailing whitespace is ignored for any rule except <literals>.

public:
    SMYTH_IMMOVABLE(SmythCharacterMap);
    SmythCharacterMap(QWidget* parent) : QWidget(parent) {}

    /// Zooming.
    void keyPressEvent(QKeyEvent* event) override {
        if (common::HandleZoomEvent(this, event)) return;
        QWidget::keyPressEvent(event);
    }

    /// Handle selecting a character.
    void mousePressEvent(QMouseEvent* event) override;

    /// Draw the character map.
    void paintEvent(QPaintEvent* event) override;

    /// Resize the character map.
    void resizeEvent(QResizeEvent* event) override;

    /// Set the font.
    void setFont(const QFont& font);

    /// Tell the scroll area to scroll us vertically.
    auto sizeHint() const -> QSize override;

    /// Zooming.
    void wheelEvent(QWheelEvent* event) override {
        if (common::HandleZoomEvent(this, event)) return;
        QWidget::wheelEvent(event);
    }

signals:
    /// A character was selected.
    void selected(char32_t);

private:
    /// Get the minimum height we need.
    auto MinHeight() const -> int;

    /// Get the number of rows we need.
    auto Rows() const -> int;

    /// Update the characters to be drawn.
    void UpdateChars();

    /// Update the size of the character map.
    void UpdateSize();
};
} // namespace smyth::ui

#endif // SMYTH_UI_SMYTHCHARACTERMAP_HH

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

    /// Cache the last query so that we can re-execute it when the font changes.
    QString last_query;

    /// All characters that the current font can display. We also
    /// store a copy of all of them as a QString for rendering.
    std::vector<char32_t> all_codepoints;
    std::vector<QString> all_chars;

    /// The characters that are currently being displayed. If empty,
    /// display all_chars instead.
    std::vector<char32_t> matched_codepoints;
    std::vector<QString> matched_chars;

    struct Range {
        char32_t from; ///< 0 if no lower bound.
        char32_t to;   ///< 0 if no upper bound.
    };

    struct Codepoint {
        char32_t value;
    };

    struct Name {
        QString value;
    };

    struct Literal {
        QString value;
    };

    using Query = std::variant<Range, Codepoint, Name, Literal, std::monostate>;

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

public slots:
    void search(QString query);

signals:
    /// A character was selected.
    void selected(char32_t);

private:
    /// Get whichever vector of chars/codepoints is currently being displayed.
    auto DisplayedChars() const -> const std::vector<QString>&;
    auto DisplayedCodepoints() const -> const std::vector<char32_t>&;

    /// Get the minimum height we need.
    auto MinHeight() const -> int;

    /// Get the number of rows we need.
    auto Rows() const -> int;

    /// Parse a query string.
    auto ParseQuery(QStringView query) -> Query;

    /// Apply a query string to the character map.
    void ProcessQuery(QStringView query);

    /// Update the characters to be drawn.
    void UpdateChars();

    /// Update the size of the character map.
    void UpdateSize();
};
} // namespace smyth::ui

#endif // SMYTH_UI_SMYTHCHARACTERMAP_HH

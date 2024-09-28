#ifndef SMYTH_UI_SMYTHCHARACTERMAP_HH
#define SMYTH_UI_SMYTHCHARACTERMAP_HH

#include <base/Text.hh>
#include <QListView>
#include <UI/Mixins.hh>

namespace smyth::ui {
class SmythCharacterMap;
} // namespace smyth::ui

class smyth::ui::SmythCharacterMap final : public QWidget
    , mixins::Zoom {
    Q_OBJECT

    friend Zoom;

    /// First printable character.
    static constexpr c32 FirstPrintChar = U' ';

    int cols{20};
    int square_height{40};
    int square_width{40};
    int selected_idx{-1};

    /// Cache the last query so that we can re-execute it when the font changes.
    QString last_query;

    /// All characters that the current font can display. We also
    /// store a copy of all of them as a QString for rendering.
    std::vector<c32> all_codepoints;
    std::vector<QString> all_chars;

    /// The characters that are currently being displayed. If empty,
    /// display all_chars instead.
    std::vector<c32> matched_codepoints;
    std::vector<QString> matched_chars;

    struct Range {
        c32 from; ///< 0 if no lower bound.
        c32 to;   ///< 0 if no upper bound.
    };

    struct Codepoint {
        c32 value;
    };

    struct Name {
        QString value;
    };

    struct Literal {
        QString value;
    };

    using Query = std::variant<Range, Codepoint, Name, Literal, std::monostate>;

public:
    LIBBASE_IMMOVABLE(SmythCharacterMap);
    SmythCharacterMap(QWidget* parent) : QWidget(parent) {}

    /// Zooming.
    void keyPressEvent(QKeyEvent* event) override {
        if (HandleZoomEvent(event)) return;
        QWidget::keyPressEvent(event);
    }

    /// Copy a character to the clipboard.
    void mouseDoubleClickEvent(QMouseEvent* event) override;

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
        if (HandleZoomEvent(event)) return;
        QWidget::wheelEvent(event);
    }

public slots:
    void search(QString query);

signals:
    /// A character was selected.
    void selected(c32);

private:
    /// Get a character index from a click.
    auto ClickToIndex(QMouseEvent* event) -> std::optional<int>;

    /// Collect all characters in a range that a font actually supports.
    void CollectChars(
        const QFontMetrics& m,
        c32 from,
        c32 to,
        std::vector<QString>& chars,
        std::vector<c32>& codepoints
    );

    /// Get whichever vector of chars/codepoints is currently being displayed.
    auto DisplayedChars() const -> const std::vector<QString>&;
    auto DisplayedCodepoints() const -> const std::vector<c32>&;

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

#endif // SMYTH_UI_SMYTHCHARACTERMAP_HH

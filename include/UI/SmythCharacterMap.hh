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
    static constexpr char16_t FirstPrintChar = ' ';

    int cols{20};
    int square_height{40};
    int square_width{40};
    int last_codepoint{1'000};
    std::vector<QChar> chars;

public:
    SMYTH_IMMOVABLE(SmythCharacterMap);
    SmythCharacterMap(QWidget* parent) : QWidget(parent) {}

    /// Zooming.
    void keyPressEvent(QKeyEvent* event) override {
        if (common::HandleZoomEvent(this, event)) return;
        QWidget::keyPressEvent(event);
    }

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

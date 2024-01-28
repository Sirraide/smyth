#ifndef SMYTH_UI_TEXTPREVIEWDIALOG_HH
#define SMYTH_UI_TEXTPREVIEWDIALOG_HH

#include <QDialog>
#include <UI/App.hh>

QT_BEGIN_NAMESPACE
namespace Ui {
class TextPreviewDialog;
}
QT_END_NAMESPACE

namespace smyth::ui {
class TextPreviewDialog final : public QDialog {
    Q_OBJECT

    std::unique_ptr<Ui::TextPreviewDialog> ui;

public:
    SMYTH_IMMOVABLE(TextPreviewDialog);
    TextPreviewDialog(QWidget* parent);
    ~TextPreviewDialog() noexcept;

    static void Show(
        const QString& title,
        const QString& text,
        const QFont& font,
        QWidget* parent = nullptr
    );

public slots:
};
} // namespace smyth::ui

#endif // SMYTH_UI_TEXTPREVIEWDIALOG_HH

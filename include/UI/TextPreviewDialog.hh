#ifndef SMYTH_UI_TEXTPREVIEWDIALOG_HH
#define SMYTH_UI_TEXTPREVIEWDIALOG_HH

#include <QDialog>
#include <UI/Smyth.hh>

QT_BEGIN_NAMESPACE
namespace Ui {
class TextPreviewDialog;
}
QT_END_NAMESPACE

namespace smyth::ui {
class TextPreviewDialog;
} // namespace smyth::ui

class smyth::ui::TextPreviewDialog final : public QDialog {
    Q_OBJECT

    std::unique_ptr<Ui::TextPreviewDialog> ui;

public:
    LIBBASE_IMMOVABLE(TextPreviewDialog);
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

#endif // SMYTH_UI_TEXTPREVIEWDIALOG_HH

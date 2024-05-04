#include <UI/TextPreviewDialog.hh>
#include <ui_TextPreviewDialog.h>

smyth::ui::TextPreviewDialog::~TextPreviewDialog() noexcept = default;

smyth::ui::TextPreviewDialog::TextPreviewDialog(QWidget* parent)
    : QDialog(parent), ui(std::make_unique<Ui::TextPreviewDialog>()) {
    ui->setupUi(this);
}

void smyth::ui::TextPreviewDialog::Show(
    const QString& title,
    const QString& text,
    const QFont& font,
    QWidget* parent
) {
    TextPreviewDialog dialog{parent};
    dialog.ui->contents->setPlainText(text);
    dialog.ui->contents->setFont(font);
    dialog.setWindowTitle(title);
    dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    dialog.exec();
}

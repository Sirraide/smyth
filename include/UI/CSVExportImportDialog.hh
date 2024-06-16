#ifndef SMYTH_UI_CSV_EXPORT_IMPORT_DIALOG_HH
#define SMYTH_UI_CSV_EXPORT_IMPORT_DIALOG_HH

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class CSVExportImportDialog;
}
QT_END_NAMESPACE

namespace smyth::ui {
class CSVExportImportDialog;
}

class smyth::ui::CSVExportImportDialog : public QDialog {
    Q_OBJECT

public:
    CSVExportImportDialog(bool is_export_dialog, QWidget* parent = nullptr);
    ~CSVExportImportDialog() override;

private:
    std::unique_ptr<Ui::CSVExportImportDialog> ui;
};

#endif // SMYTH_UI_CSV_EXPORT_IMPORT_DIALOG_HH

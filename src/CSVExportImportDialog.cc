#include <UI/CSVExportImportDialog.hh>
#include <ui_CSVImportExportDialog.h>

using namespace smyth;
using namespace smyth::ui;

CSVExportImportDialog::~CSVExportImportDialog() = default;
CSVExportImportDialog::CSVExportImportDialog(bool is_export_dialog, QWidget* parent)
    : QDialog(parent),
      ui(std::make_unique<Ui::CSVExportImportDialog>()) {
    ui->setupUi(this);
    if (is_export_dialog) {
        ui->check_header_line->setText("Include header");
        ui->check_enclosed_by->setChecked(false);
        ui->check_short_lines->setDisabled(true);
    }
}

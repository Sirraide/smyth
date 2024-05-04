#include <QApplication>
#include <QMessageBox>
#include <UI/App.hh>
#include <UI/MainWindow.hh>

void ShowError(std::string message, smyth::ErrorMessageType type) {
    using Ty = smyth::ErrorMessageType;
    auto icon = type == Ty::Info  ? QMessageBox::Information
              : type == Ty::Error ? QMessageBox::Warning
                                  : QMessageBox::Critical;

    auto title = type == Ty::Info  ? "Info"
               : type == Ty::Error ? "Error"
                                   : "Fatal Error";

    /// Fatal errors may be due to errors during painting etc., so yeet
    /// everything to make sure we don’t end up causing multiple fatal
    /// errors in a row.
    if (type == Ty::Fatal) {
        QApplication::setQuitOnLastWindowClosed(false);
        QApplication::closeAllWindows();
    }

    QMessageBox box;
    box.setWindowTitle(title);
    box.setIcon(icon);
    box.setModal(true);
    box.setText(QString::fromStdString(message));
    box.setWindowFlags(box.windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint));
    box.exec();
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Smyth");
    QCoreApplication::setApplicationName("Smyth");
    QCoreApplication::setOrganizationDomain("nguh.org");
    smyth::ui::App Smyth{ShowError};
    return app.exec();
}

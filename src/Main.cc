#include <App.hh>
#include <MainWindow.hh>
#include <QApplication>
#include <QMessageBox>

void ShowError(std::string message, smyth::ErrorMessageType type) {
    using Ty = smyth::ErrorMessageType;
    auto icon = type == Ty::Info  ? QMessageBox::Information
              : type == Ty::Error ? QMessageBox::Warning
                                  : QMessageBox::Critical;

    auto title = type == Ty::Info  ? "Info"
               : type == Ty::Error ? "Error"
                                   : "Fatal Error";

    QMessageBox box;
    box.setWindowTitle(title);
    box.setIcon(icon);
    box.setText(QString::fromStdString(message));
    box.setWindowFlags(box.windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint));
    box.exec();
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Smyth");
    QCoreApplication::setApplicationName("Smyth");
    QCoreApplication::setOrganizationDomain("nguh.org");
    smyth::App Smyth;
    smyth::RegisterMessageHandler(ShowError);
    Smyth.main_window()->show();
    return app.exec();
}

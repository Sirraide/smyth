#include <App.hh>
#include <MainWindow.hh>
#include <QApplication>
#include <QMessageBox>

void ShowError(std::string message) {
    QMessageBox box;
    box.setWindowTitle("Error");
    box.setIcon(QMessageBox::Critical);
    box.setText(QString::fromStdString(message));
    box.setWindowFlags(box.windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint));
    box.exec();
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    smyth::App Smyth;
    smyth::RegisterMessageHandler(ShowError);
    smyth::MainWindow w{Smyth};
    w.show();
    return app.exec();
}

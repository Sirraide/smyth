#include <QApplication>
#include <UI/MainWindow.hh>
#include <UI/Smyth.hh>

void FailureHandler(const libassert::assertion_info& info) {
    /// Fatal errors may be due to errors during painting etc., so yeet
    /// everything to make sure we don’t end up causing multiple fatal
    /// errors in a row.
    std::print("{}\n", info.to_string());
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::closeAllWindows();
    QMessageBox box;
    box.setWindowTitle("Fatal Error");
    box.setIcon(QMessageBox::Critical);
    box.setModal(true);
    box.setText(QString::fromStdString(info.to_string(80, libassert::color_scheme::blank)));
    box.setWindowFlags(box.windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint));
    box.exec();
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Smyth");
    QCoreApplication::setApplicationName("Smyth");
    QCoreApplication::setOrganizationDomain("nguh.org");
    libassert::set_failure_handler(FailureHandler);
    smyth::ui::InitialiseSmyth();
    return app.exec();
}

#ifndef SMYTH_MAINWINDOW_HH
#define SMYTH_MAINWINDOW_HH

#include <App.hh>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace smyth {
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    SMYTH_IMMOVABLE(MainWindow);
    MainWindow(App& app);
    ~MainWindow() noexcept;

private:
    std::unique_ptr<Ui::MainWindow> ui;
};
} // namespace smyth
#endif // SMYTH_MAINWINDOW_HH

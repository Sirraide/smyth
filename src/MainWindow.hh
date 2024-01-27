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

    smyth::App& app;
    std::unique_ptr<Ui::MainWindow> ui;

public:
    SMYTH_IMMOVABLE(MainWindow);
    MainWindow(App& app);
    ~MainWindow() noexcept;

public slots:
    void apply_sound_changes();
    void open_project();
    void save_project();
};
} // namespace smyth
#endif // SMYTH_MAINWINDOW_HH

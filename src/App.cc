#include <App.hh>
#include <QMessageBox>

auto smyth::App::applySoundChanges(QString inputs, QString sound_changes) -> QString {
    auto res = lexurgy(inputs, sound_changes);
    if (res.is_err()) {
        ShowError(QString::fromStdString(res.err().message));
        return "ERROR";
    }

    return res.value();
}

void smyth::App::save() {
    ShowError("TODO: Implement save()");
}

void smyth::App::ShowError(QString message) {
    QMessageBox box;
    box.setWindowTitle("Error");
    box.setIcon(QMessageBox::Critical);
    box.setText(message);
    box.setWindowFlags(box.windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint));
    box.exec();
}
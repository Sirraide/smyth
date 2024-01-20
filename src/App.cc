#include <App.hh>

auto smyth::App::applySoundChanges(QString inputs, QString sound_changes) -> QString {
    auto res = lexurgy(inputs, sound_changes);
    if (res.is_err()) {
        emit showErrorDialog(QString::fromStdString(res.err().message));
        return "ERROR";
    }

    return res.value();
}

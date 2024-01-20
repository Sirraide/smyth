#ifndef SMYTH_APP_HH
#define SMYTH_APP_HH

#include <QObject>
#include <Utils.hh>
#include <Lexurgy.hh>

namespace smyth {

class App : public QObject {
    Q_OBJECT

    Lexurgy lexurgy;

signals:
    void init();
    void showErrorDialog(QString message);

public slots:
    /// Apply sound changes to the input string.
    auto applySoundChanges(QString inputs, QString sound_changes) -> QString;
};

}
#endif // SMYTH_APP_HH

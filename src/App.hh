#ifndef SMYTH_APP_HH
#define SMYTH_APP_HH

#include <QObject>
#include <Utils.hh>
#include <Lexurgy.hh>

namespace smyth {

class App : public QObject {
    Q_OBJECT

    Lexurgy lexurgy;

public:
    static void ShowError(QString message);

signals:
    void init();

public slots:
    /// Apply sound changes to the input string.
    auto applySoundChanges(QString inputs, QString sound_changes) -> QString;

    /// Save project to a file.
    void save();
};

}
#endif // SMYTH_APP_HH

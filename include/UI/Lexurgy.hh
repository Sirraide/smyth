#ifndef SMYTH_UI_LEXURGY_HH
#define SMYTH_UI_LEXURGY_HH

#include <QString>
#include <Smyth/Utils.hh>

namespace smyth::lexurgy {
/// Apply sound changes.
auto Apply(
    QStringView words,
    QString changes,
    const QString& stop_before
) -> Result<QString>;

/// Close the lexurgy process.
void Close();
} // namespace smyth::lexurgy


#endif //SMYTH_UI_LEXURGY_HH

#ifndef SMYTH_LEXURGY_HH
#define SMYTH_LEXURGY_HH

#include <QProcess>
#include <Result.hh>
#include <Utils.hh>

namespace smyth {
/// Lexurgy connexion.
class Lexurgy {
    QProcess lexurgy_process;

    /// Cache the sound changes to avoid pointless requests.
    QString sound_changes;

public:
    Lexurgy(const Lexurgy&) = delete;
    Lexurgy(Lexurgy&&) = delete;
    auto operator=(const Lexurgy&) -> Lexurgy& = delete;
    auto operator=(Lexurgy&&) -> Lexurgy& = delete;

    /// Create a new lexurgy connexion.
    Lexurgy();

    /// Tear down the connexion.
    ~Lexurgy();

    /// Apply sound changes.
    auto operator()(
        QStringView words,
        QString changes,
        const QString& stop_before
    ) -> Result<QString>;

private:
    /// Make a request to lexurgy.
    template <typename Res, typename Req>
    auto SendRequest(Req&& r) -> Result<Res>;

    /// Update sound changes.
    auto UpdateSoundChanges(QString changes) -> Result<>;
};
} // namespace smyth

#endif // SMYTH_LEXURGY_HH

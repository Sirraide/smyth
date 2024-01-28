#ifndef SMYTH_UI_LEXURGY_HH
#define SMYTH_UI_LEXURGY_HH

#include <QProcess>
#include <Smyth/Result.hh>
#include <Smyth/Utils.hh>

namespace smyth::ui {
/// Lexurgy connexion.
class Lexurgy {
    QProcess lexurgy_process;

    /// Cache the sound changes to avoid pointless requests.
    std::optional<QString> sound_changes;

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
    auto apply(
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
} // namespace smyth::ui

#endif // SMYTH_UI_LEXURGY_HH

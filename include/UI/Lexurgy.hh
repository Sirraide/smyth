#ifndef SMYTH_UI_LEXURGY_HH
#define SMYTH_UI_LEXURGY_HH

#include <QProcess>
#include <Smyth/Utils.hh>

namespace smyth::ui {
class Lexurgy;
} // namespace smyth::ui

/// Lexurgy connexion.
class smyth::ui::Lexurgy {
    QProcess lexurgy_process;

    /// Cache the sound changes to avoid pointless requests.
    std::optional<QString> sound_changes;

public:
    LIBBASE_IMMOVABLE(Lexurgy);
    ~Lexurgy();

    /// Try to create a new process and wait until it has started.
    static auto Start() -> Result<std::unique_ptr<Lexurgy>>;

    /// Apply sound changes.
    auto apply(
        QStringView words,
        QString changes,
        const QString& stop_before
    ) -> Result<QString>;

private:
    Lexurgy();

    /// Update sound changes.
    auto UpdateSoundChanges(QString changes) -> Result<>;
};

#endif // SMYTH_UI_LEXURGY_HH

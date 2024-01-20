#ifndef SMYTH_LEXURGY_HH
#define SMYTH_LEXURGY_HH

#include <Utils.hh>
#include <QProcess>
#include <Result.hh>

namespace smyth {
/// Lexurgy connexion.
class Lexurgy {
    QProcess lexurgy_process;

    /// Cache the sound changes to avoid pointless requests.
    QString sound_changes;

public:
    /// Error returned from lexurgy.
    struct Error {
        std::string message;
    };

    Lexurgy(const Lexurgy&) = delete;
    Lexurgy(Lexurgy&&) = delete;
    auto operator=(const Lexurgy&) -> Lexurgy& = delete;
    auto operator=(Lexurgy&&) -> Lexurgy& = delete;

    /// Create a new lexurgy connexion.
    Lexurgy();

    /// Tear down the connexion.
    ~Lexurgy();

    /// Apply sound changes.
    auto operator()(QStringView words, QString changes) -> Result<QString, Error>;

private:
    /// Make a request to lexurgy.
    template <typename Res, typename Req>
    auto SendRequest(Req&& r) -> Result<Res, Error>;

    /// Update sound changes.
    auto UpdateSoundChanges(QString changes) -> Result<void, Error>;
};
}


#endif // SMYTH_LEXURGY_HH

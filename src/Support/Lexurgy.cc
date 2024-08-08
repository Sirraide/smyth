module;

#include <base/Macros.hh>
#include <base/Result.hh>
#include <print>
#include <QProcess>
#include <ranges>
#include <Smyth/JSON.hh>

module smyth.lexurgy;
// import smyth.json;

using namespace smyth;
using namespace smyth::Lexurgy;
using json = json_utils::json;

/// Lexurgy connexion.
struct Connexion {
    LIBBASE_IMMOVABLE(Connexion);

    QProcess lexurgy_process;

    /// Cache the sound changes to avoid pointless requests.
    std::optional<QString> sound_changes;

    Connexion() { lexurgy_process.start(LEXURGY_ROOT "/bin/lexurgy", QStringList() << "server"); }
    ~Connexion() { lexurgy_process.close(); }

    /// Try to create a new process and wait until it has started.
    static auto Start() -> Result<std::unique_ptr<Connexion>>;

    /// Apply sound changes.
    auto apply(
        QStringView words,
        QString changes,
        const QString& stop_before
    ) -> Result<QString>;

    /// Send a request to the Lexurgy process.
    auto SendRequest(const json& request) -> Result<json>;

    /// Update sound changes.
    auto UpdateSoundChanges(QString changes) -> Result<>;
};

/// The global lexurgy connexion.
static std::unique_ptr<Connexion> ConnexionPointer;

auto CreateConnexion() -> Result<> {
    ConnexionPointer.reset(new Connexion);
    if (ConnexionPointer->lexurgy_process.waitForStarted(5'000)) return {};

    // Something went wrong. Throw this away.
    ConnexionPointer.reset();
    return Error(
        "Failed to start lexurgy process. Expected lexurgy at '{}'",
        LEXURGY_ROOT "/bin/lexurgy"
    );
}

auto GetConnexion() -> Result<Connexion&> {
    if (not ConnexionPointer) Try(CreateConnexion());
    return *ConnexionPointer;
}

auto Connexion::SendRequest(const json& request) -> Result<json> {
    auto req = request.dump();
#ifdef LIBBASE_DEBUG
    if (DumpJsonRequests) std::println(stderr, " -> Lexurgy: {}", req);
#endif
    lexurgy_process.write(req.data(), qint64(req.size()));
    lexurgy_process.write("\n");
    lexurgy_process.waitForReadyRead(5'000);
    auto line = lexurgy_process.readLine();
    std::string_view sv = {line.data(), usz(line.size())};

    /// Parse the response.
    auto res = Try(json_utils::Parse(sv));
    if (not res.contains("type")) return Error("Missing 'type' field in response");
    if (res["type"] == "error") {
        if (not res.contains("message")) return Error("Lexurgy error: Unknown error");
        return Error("Lexurgy error: {}", res["message"].get<std::string>());
    }
    return res;
}

auto Connexion::UpdateSoundChanges(QString changes) -> Result<> {
    auto tr = std::move(changes).trimmed();
    if (tr == sound_changes) return {};
    auto res = Try(SendRequest(json{{"type", "load_string"}, {"changes", tr.toStdString()}}));
    if (res["type"] != "ok") return Error(
        "Lexurgy error: Unexpected response type for setting sound changes '{}'",
        res["type"].get<std::string>()
    );

    sound_changes = std::move(tr);
    return {};
}

/// ====================================================================
///  API
/// ====================================================================
auto Connexion::apply(
    QStringView input,
    QString changes,
    const QString& stop_before
) -> Result<QString> {
    Try(UpdateSoundChanges(std::move(changes)));

    std::vector<std::string> words;
    for (auto w : input | vws::split('\n') | vws::filter([](auto&& w) { return not w.empty(); })) {
        auto sv = QStringView{w.begin(), w.end() - w.begin()};
        words.push_back(sv.toString().toStdString());
    }

    std::optional<std::string> stop_before_opt;
    if (stop_before != "") stop_before_opt = stop_before.toStdString();
    json req;
    req["type"] = "apply";
    req["words"] = words;
    if (stop_before_opt) req["stopBefore"] = *stop_before_opt;
    auto changed = Try(SendRequest(req));
    if (changed["type"] != "changed") return Error(
        "Lexurgy error: Unexpected response type '{}'",
        changed["type"].get<std::string>()
    );

    QString joined;
    for (const auto& w : changed["words"].get<std::vector<std::string>>()) {
        joined += w;
        joined += '\n';
    }
    return joined;
}

/// ====================================================================
///  API
/// ====================================================================
auto Lexurgy::Apply(
    QStringView words,
    QString changes,
    const QString& stop_before
) -> Result<QString> {
    Connexion& conn = Try(GetConnexion());
    return conn.apply(words, std::move(changes), stop_before);
}

void Lexurgy::Reset() {
    ConnexionPointer.reset();
}

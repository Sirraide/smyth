#include <QProcess>
#include <ranges>
#include <Smyth/JSON.hh>
#include <Smyth/Utils.hh>
#include <UI/Lexurgy.hh>
#include <UI/Smyth.hh>

using namespace smyth;
using namespace smyth::ui;
using json = json_utils::json;

namespace {
class Connexion {
    LIBBASE_IMMOVABLE(Connexion);
    QProcess lexurgy_process;

    /// Cache the sound changes to avoid pointless requests.
    std::optional<QString> sound_changes;

    /// Unique ptr so we can replace it.
    static std::unique_ptr<Connexion> Instance;

    Connexion() = default;

public:
    ~Connexion();

    /// Close the connexion.
    static void Close();

    /// Try to create a new process and wait until it has started.
    static auto Get() -> Result<Connexion&>;

    /// Apply sound changes.
    auto Apply(
        QStringView input,
        QString changes,
        const QString& stop_before
    ) -> Result<QString>;

    /// Update sound changes.
    auto UpdateSoundChanges(QString changes) -> Result<>;


private:
    auto SendRequest(const json& request) -> Result<json>;
    static auto Start() -> Result<std::unique_ptr<Connexion>>;
};

std::unique_ptr<Connexion> Connexion::Instance;

Connexion::~Connexion() { lexurgy_process.close(); }

auto Connexion::Apply(
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

void Connexion::Close() {
    Instance.reset();
}

auto Connexion::Get() -> Result<Connexion&> {
    if (not Instance) Instance = Try(Start());
    return *Instance;
}

auto Connexion::SendRequest(const json& request) -> Result<json> {
    auto req = request.dump();
#ifdef LIBBASE_DEBUG
    if (*settings::DumpJsonRequests) std::println(stderr, " -> Lexurgy: {}", req);
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

auto Connexion::Start() -> Result<std::unique_ptr<Connexion>> {
    std::unique_ptr<Connexion> ptr{new Connexion};
    ptr->lexurgy_process.start(LEXURGY_ROOT "/bin/lexurgy", QStringList() << "server");
    if (not ptr->lexurgy_process.waitForStarted(5'000)) return Error(
        "Failed to start lexurgy process. Expected lexurgy at '{}'",
        LEXURGY_ROOT "/bin/lexurgy"
    );
    return std::move(ptr);
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
} // namespace

// =====================================================================
//  API
// =====================================================================
auto lexurgy::Apply(
    QStringView input,
    QString changes,
    const QString& stop_before
) -> Result<QString> {
    return Try(Connexion::Get())->Apply(input, std::move(changes), stop_before);
}

void lexurgy::Close() {
    Connexion::Close();
}

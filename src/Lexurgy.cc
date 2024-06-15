#include <ranges>
#include <Smyth/JSON.hh>
#include <Smyth/Utils.hh>
#include <UI/App.hh>
#include <UI/Lexurgy.hh>

using namespace smyth;
using namespace smyth::ui;
using json = json_utils::json;

static auto SendRequest(QProcess& lexurgy_process, const json& request) -> Result<json> {
    auto req = request.dump();
#ifdef LIBBASE_DEBUG
    if (*App::The().dump_json_requests) std::println(stderr, " -> Lexurgy: {}", req);
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

/// ====================================================================
///  Lexurgy – Implementation
/// ====================================================================
auto Lexurgy::Start() -> Result<std::unique_ptr<Lexurgy>> {
    std::unique_ptr<Lexurgy> ptr{new Lexurgy};
    if (not ptr->lexurgy_process.waitForStarted(5'000)) return Error(
        "Failed to start lexurgy process. Expected lexurgy at '{}'",
        LEXURGY_ROOT "/bin/lexurgy"
    );
    return std::move(ptr);
}

auto Lexurgy::UpdateSoundChanges(QString changes) -> Result<> {
    auto tr = std::move(changes).trimmed();
    if (tr == sound_changes) return {};
    auto res = Try(SendRequest(lexurgy_process, json{{"type", "load_string"}, {"changes", tr.toStdString()}}));
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
Lexurgy::Lexurgy() {
    lexurgy_process.start(LEXURGY_ROOT "/bin/lexurgy", QStringList() << "server");
}

Lexurgy::~Lexurgy() {
    lexurgy_process.close();
}

auto Lexurgy::apply(
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
    auto changed = Try(SendRequest(lexurgy_process, req));
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

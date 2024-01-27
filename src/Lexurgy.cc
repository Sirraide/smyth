#include <glaze/glaze.hpp>
#include <Lexurgy.hh>
#include <ranges>
#include <Utils.hh>

namespace smyth::detail::lexurgy_requests { // clang-format off
struct ApplyRequest {
    std::vector<std::string> words;
    std::optional<std::string> stopBefore;
    std::string_view type = "apply";
};

struct LoadStringRequest {
    std::string changes;
    std::string_view type = "load_string";
};

struct ErrorResponse { std::string message; };
struct ChangedResponse { std::vector<std::string> words; };
using OkResponse = std::monostate;
using Response = std::variant<ChangedResponse, ErrorResponse, OkResponse>;

static constexpr glz::opts IgnoreUnknown{.error_on_unknown_keys = false};
} // clang-format on

using namespace smyth::detail::lexurgy_requests;
using namespace std::literals;

template <>
struct glz::meta<Response> {
    static constexpr std::string_view tag = "type";
    static constexpr std::array ids = {"changed"sv, "error"sv, "ok"sv};
};

/// ====================================================================
///  Lexurgy – Implementation
/// ====================================================================
template <typename Res, typename Req>
auto smyth::Lexurgy::SendRequest(Req&& r) -> Result<Res, Error> {
    auto s = glz::write_json(std::move(r));
    lexurgy_process.write(s.data(), qint64(s.size()));
    lexurgy_process.write("\n");
    lexurgy_process.waitForReadyRead(5'000);
    auto line = lexurgy_process.readLine();
    std::string_view sv = {line.data(), usz(line.size())};

    /// Parse the response.
    Response res{};
    if (auto e = glz::read<IgnoreUnknown>(res, sv)) return Lexurgy::Error(
        fmt::format("Error parsing JSON response: {}", glz::format_error(e, sv))
    );

    /// Handle errors.
    if (auto err = std::get_if<ErrorResponse>(&res))
        return Lexurgy::Error(fmt::format("Lexurgy error: {}", err->message));

    /// Make sure the response is of the right type.
    if (auto val = std::get_if<Res>(&res)) return std::move(*val);
    else return Lexurgy::Error(fmt::format("Unexpected response type '{}'", sv));
}

auto smyth::Lexurgy::UpdateSoundChanges(QString changes) -> Result<void, Error> {
    auto tr = std::move(changes).trimmed();
    if (tr == sound_changes) return {};
    sound_changes = std::move(tr);

    /// Update the sound changes.
    auto res = SendRequest<OkResponse>(LoadStringRequest{changes.toStdString()});
    if (res.is_err()) {
        sound_changes.clear();
        return res.err();
    }

    return {};
}

/// ====================================================================
///  API
/// ====================================================================
smyth::Lexurgy::Lexurgy() {
    lexurgy_process.start(LEXURGY_ROOT "/bin/lexurgy", QStringList() << "server");
    if (not lexurgy_process.waitForStarted(5'000))
        Fatal("Failed to start lexurgy process.");
}

smyth::Lexurgy::~Lexurgy() {
    lexurgy_process.close();
}

auto smyth::Lexurgy::operator()(
    QStringView input,
    QString changes,
    const QString& stop_before
) -> Result<QString, Error> {
    if (auto res = UpdateSoundChanges(std::move(changes)); not res)
        return res.err();

    std::vector<std::string> words;
    for (auto w : input | vws::split('\n') | vws::filter([](auto&& w) { return not w.empty(); })) {
        auto sv = QStringView{w.begin(), w.end() - w.begin()};
        words.push_back(sv.toString().toStdString());
    }

    std::optional<std::string> stop_before_opt;
    if (stop_before != "") stop_before_opt = stop_before.toStdString();
    auto res = SendRequest<ChangedResponse>(ApplyRequest{std::move(words), std::move(stop_before_opt)});
    if (res.is_err()) return res.err();

    QString joined;
    for (const auto& w : res->words) {
        joined += w;
        joined += '\n';
    }
    return joined;
}

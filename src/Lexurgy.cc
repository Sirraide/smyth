#include "UI/SettingsDialog.hh"

#include <glaze/glaze.hpp>
#include <ranges>
#include <Smyth/Utils.hh>
#include <UI/App.hh>
#include <UI/Lexurgy.hh>

namespace smyth::ui::detail::lexurgy_requests { // clang-format off
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

using namespace smyth::ui::detail::lexurgy_requests;
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
auto smyth::ui::Lexurgy::SendRequest(Req&& r) -> Result<Res> {
    auto s = glz::write_json(std::move(r));
    std::string_view req{s.data(), s.size()};
    if (App::The().settings_dialog()->show_json_requests())
        Debug(" -> Lexurgy: {}", req);
    lexurgy_process.write(req.data(), qint64(req.size()));
    lexurgy_process.write("\n");
    lexurgy_process.waitForReadyRead(5'000);
    auto line = lexurgy_process.readLine();
    std::string_view sv = {line.data(), usz(line.size())};

    /// Parse the response.
    Response res{};
    if (auto e = glz::read<IgnoreUnknown>(res, sv)) return Err(
        "Error parsing JSON response: {}",
        glz::format_error(e, sv)
    );

    /// Handle errors.
    if (auto err = std::get_if<ErrorResponse>(&res))
        return Err("Lexurgy error: {}", err->message);

    /// Make sure the response is of the right type.
    if (auto val = std::get_if<Res>(&res)) return std::move(*val);
    return Err("Unexpected response type '{}'", sv);
}

auto smyth::ui::Lexurgy::UpdateSoundChanges(QString changes) -> Result<> {
    auto tr = std::move(changes).trimmed();
    if (tr == sound_changes) return {};
    Try(SendRequest<OkResponse>(LoadStringRequest{changes.toStdString()}));
    sound_changes = std::move(tr);
    return {};
}

/// ====================================================================
///  API
/// ====================================================================
smyth::ui::Lexurgy::Lexurgy() {
    lexurgy_process.start(LEXURGY_ROOT "/bin/lexurgy", QStringList() << "server");
    if (not lexurgy_process.waitForStarted(5'000)) Fatal(
        "Failed to start lexurgy process. Expected lexurgy at '{}'",
        LEXURGY_ROOT "/bin/lexurgy"
    );
}

smyth::ui::Lexurgy::~Lexurgy() {
    lexurgy_process.close();
}

auto smyth::ui::Lexurgy::apply(
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
    auto changed = Try(SendRequest<ChangedResponse>(ApplyRequest{std::move(words), std::move(stop_before_opt)}));

    QString joined;
    for (const auto& w : changed.words) {
        joined += w;
        joined += '\n';
    }
    return joined;
}

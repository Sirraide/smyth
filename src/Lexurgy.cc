#include <glaze/glaze.hpp>
#include <Lexurgy.hh>
#include <ranges>

#define LEXURGY_REQUEST_APPLY       "apply"
#define LEXURGY_REQUEST_LOAD_STRING "load_string"
#define LEXURGY_RESPONSE_ERROR      "error"
#define LEXURGY_RESPONSE_CHANGED    "changed"
#define LEXURGY_RESPONSE_OK         "ok"

struct LoadStringRequest {
    std::string_view type = LEXURGY_REQUEST_LOAD_STRING;
    struct {
        std::string changes = "foo";
    } data;
};

struct ApplyRequest {
    std::string_view type = LEXURGY_REQUEST_APPLY;
    struct {
        std::vector<std::string> words;
    } data;
};

struct ChangedResponse {
    std::vector<std::string> words;
};

struct OkResponse {};

namespace smyth {
namespace {
template <typename T>
auto ReadPartial(std::string_view from, glz::context& ctx) -> Result<T, Lexurgy::Error> {
    T into;
    auto e = glz::read<glz::opts{.error_on_unknown_keys = false}>(
        into,
        std::forward<decltype(from)>(from),
        ctx
    );

    if (e) return Lexurgy::Error(fmt::format("Error parsing JSON response: {}", glz::format_error(e, from)));
    return std::move(into);
}
} // namespace
} // namespace smyth

/// ====================================================================
///  Lexurgy – Implementation
/// ====================================================================
template <typename Res, typename Req>
auto smyth::Lexurgy::SendRequest(Req&& r) -> Result<Res, Error> {
    struct ResponseTag {
        std::string type;
    };

    struct ErrorResponse {
        std::string message;
        std::vector<std::string> stackTrace;
    };

    auto s = glz::write_json(std::move(r));
    lexurgy_process.write(s.data(), qint64(s.size()));
    lexurgy_process.write("\n");
    lexurgy_process.waitForReadyRead(5'000);
    auto line = lexurgy_process.readLine();
    std::string_view sv = {line.data(), usz(line.size())};

    /// Parse the response.
    glz::context ctx;
    auto response_tag = ReadPartial<ResponseTag>(sv, ctx);
    if (response_tag.is_err()) return response_tag.err();

    /// Check the tag to figure out what it is.
    if (response_tag->type == LEXURGY_RESPONSE_ERROR) {
        auto err = ReadPartial<ErrorResponse>(sv, ctx);
        if (err.is_err()) return err.err();
        return Error(std::move(err->message));
    }

    if (response_tag->type == LEXURGY_RESPONSE_OK) {
        if constexpr (not std::is_same_v<Res, OkResponse>) return Error(fmt::format("Unexpected 'ok' response", sv));
        else return {};
    }

    if (response_tag->type == LEXURGY_RESPONSE_CHANGED) {
        if constexpr (not std::is_same_v<Res, ChangedResponse>) return Error(fmt::format("Unexpected 'changed' response", sv));
        else {
            auto res = ReadPartial<ChangedResponse>(sv, ctx);
            if (res.is_err()) return res.err();
            return std::move(res);
        }
    }

    return Error(fmt::format("Unknown response type '{}'", response_tag->type));
}

auto smyth::Lexurgy::UpdateSoundChanges(QString changes) -> Result<void, Error> {
    auto tr = std::move(changes).trimmed();
    if (tr == sound_changes) return {};
    sound_changes = std::move(tr);

    /// Update the sound changes.
    auto res = SendRequest<OkResponse>(LoadStringRequest{
        .data = {.changes = changes.toStdString()},
    });

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
        die("Failed to start lexurgy process.");
}

smyth::Lexurgy::~Lexurgy() {
    lexurgy_process.close();
}

auto smyth::Lexurgy::operator()(
    QStringView input,
    QString changes
) -> Result<QString, Error> {
    if (auto res = UpdateSoundChanges(std::move(changes)); not res)
        return res.err();

    std::vector<std::string> words;
    for (auto w : input | vws::split('\n') | vws::filter([](auto&& w) { return not w.empty(); })) {
        auto sv = QStringView{w.begin(), w.end() - w.begin()};
        words.push_back(sv.toString().toStdString());
    }

    auto res = SendRequest<ChangedResponse>(ApplyRequest{
        .data = {.words = std::move(words)}
    });
    if (res.is_err()) return res.err();

    QString joined;
    for (const auto& w : res->words) {
        joined += w;
        joined += '\n';
    }
    return joined;
}

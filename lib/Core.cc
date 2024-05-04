#include <algorithm>
#include <atomic>
#include <mutex>
#include <Smyth/Persistent.hh>
#include <Smyth/Unicode.hh>
#include <Smyth/Utils.hh>
#include <thread>
#include <unicode/translit.h>
#include <unicode/uchar.h>

/// ====================================================================
///  Error Handling
/// ====================================================================
namespace smyth {
namespace {
void DefaultHandler(std::string message, ErrorMessageType type) {
    switch (type) {
        case ErrorMessageType::Info: fmt::print(stderr, "Info"); break;
        case ErrorMessageType::Error: fmt::print(stderr, "Error"); break;
        case ErrorMessageType::Fatal: fmt::print(stderr, "Fatal Error"); break;
    }

    fmt::print(stderr, ": {}\n", message);
}

std::atomic<ErrorMessageHandler*> handler = DefaultHandler;
} // namespace
} // namespace smyth

void smyth::detail::AssertFail(
    AssertKind k,
    std::string_view condition,
    std::string_view file,
    int line,
    std::string&& message
) {
    std::string text = [&] -> std::string {
        switch (k) {
            case AssertKind::AK_Assert: return "Assertion failed";
            case AssertKind::AK_DebugAssert: return "Debug Assertion failed";
            case AssertKind::AK_Todo: return "Not yet implemented";
            case AssertKind::AK_Unreachable: return "Unreachable code reached";
        }

        Unreachable("Invalid assert kind");
    }();

    if (not condition.empty()) text += fmt::format(": ‘{}’", condition);
    if (not message.empty()) text += fmt::format("\n  {}", message);
    text += fmt::format("\n  at {}:{}\n\n", file, line);
    text += "This is a bug in Smyth. Please file an issue at\n" SMYTH_ISSUES_URL;
    MessageImpl(std::move(text), ErrorMessageType::Fatal);
    std::exit(17);
}

void smyth::detail::MessageImpl(std::string message, ErrorMessageType type) {
    auto h = handler.load(std::memory_order_relaxed);
    if (h != nullptr) h(std::move(message), type);
}

void smyth::RegisterMessageHandler(ErrorMessageHandler* new_handler) {
    if (new_handler == nullptr) new_handler = DefaultHandler;
    handler.store(new_handler, std::memory_order_relaxed);
}

/// ====================================================================
///  Persistent
/// ====================================================================
auto smyth::PersistentStore::Entries() {
    std::vector<std::pair<std::string_view, Entry*>> sorted;
    sorted.reserve(entries.size());
    for (auto& [key, entry] : entries) sorted.emplace_back(key, &entry);
    rgs::sort(sorted, [](const auto& a, const auto& b) {
        return a.second->priority < b.second->priority;
    });
    return sorted;
}

template <typename Callback>
auto smyth::PersistentStore::ForEachEntry(
    DBRef db,
    std::string_view query,
    Callback cb
) -> Result<> {
    Try(Init(*db));
    auto stmt = Try(db->prepare(query));
    for (const auto& [key, entry] : Entries()) {
        defer { stmt.reset(); };
        stmt.bind(1, key);
        auto res = std::invoke(cb, stmt, entry->entry.get());
        if (res.is_err()) return Err("Failed to load entry '{}': {}", key, res.err());
    }
    return {};
}

auto smyth::PersistentStore::Init(Database& db) -> Result<> {
    static constexpr std::string_view query = R"sql(
        CREATE TABLE IF NOT EXISTS {} (
            key TEXT PRIMARY KEY,
            value BLOB
        ); -- *NOT* strict!!!
    )sql";

    return db.exec(fmt::format(query, table_name));
}

void smyth::PersistentStore::register_entry(std::string key, Entry entry) {
    if (entries.contains(key)) Fatal("Duplicate key '{}'", key);
    entries.emplace(std::move(key), std::move(entry));
}

auto smyth::PersistentStore::modified(DBRef db) -> Result<bool> {
    static constexpr std::string_view query = R"sql(
        SELECT value FROM {} WHERE key = ?;
    )sql";

    bool modified = false;
    auto Test = [&](Statement& stmt, detail::PersistentBase* entry) -> Result<> {
        if (modified) return {};
        auto res = Try(stmt.fetch_optional());
        if (not res.has_value()) return {};
        modified = Try(entry->modified(Column(*res, 0)));
        return {};
    };

    Try(ForEachEntry(std::move(db), fmt::format(query, table_name), Test));
    return modified;
}

auto smyth::PersistentStore::reload_all(DBRef db) -> Result<> {
    static constexpr std::string_view query = R"sql(
        SELECT value FROM {} WHERE key = ?;
    )sql";

    auto Reload = [](Statement& stmt, detail::PersistentBase* entry) -> Result<> {
        auto res = Try(stmt.fetch_optional());
        if (not res.has_value()) return {};
        return entry->load(Column(*res, 0));
    };

    return ForEachEntry(std::move(db), fmt::format(query, table_name), Reload);
}

void smyth::PersistentStore::reset_all() {
    for (const auto& [key, entry] : Entries())
        entry->entry->restore();
}

auto smyth::PersistentStore::save_all(DBRef db) -> Result<> {
    static constexpr std::string_view query = R"sql(
        INSERT INTO {} (key, value) VALUES (?, ?)
        ON CONFLICT (key) DO UPDATE SET value = EXCLUDED.value;
    )sql";

    auto Save = [](Statement& stmt, detail::PersistentBase* entry) -> Result<> {
        Try(entry->save(QueryParamRef(stmt, 2)));
        return stmt.exec();
    };

    return ForEachEntry(std::move(db), fmt::format(query, table_name), Save);
}

/// ====================================================================
///  Unicode
/// ====================================================================
auto smyth::unicode::c32::category() const -> UCharCategory {
    return UCharCategory(u_charType(UChar32(value)));
}

auto smyth::unicode::c32::name() const -> Result<std::string> {
    UErrorCode ec{U_ZERO_ERROR};
    std::array<char, 1'024> char_name{};
    auto len = u_charName(UChar32(value), U_UNICODE_CHAR_NAME, char_name.data(), i32(char_name.size()), &ec);
    if (U_FAILURE(ec)) return Err("Failed to get name for codepoint: {}", u_errorName(ec));
    return std::string{char_name.data(), usz(len)};
}

auto smyth::unicode::c32::swap_case() const -> c32 {
    auto cat = category();
    if (cat == U_UPPERCASE_LETTER) return to_lower();
    if (cat == U_LOWERCASE_LETTER) return to_upper();
    return *this;
}

auto smyth::unicode::c32::to_lower() const -> c32 {
    return u_tolower(UChar32(value));
}

auto smyth::unicode::c32::to_upper() const -> c32 {
    return u_toupper(UChar32(value));
}

auto smyth::unicode::FindCharsByName(
    std::function<bool(c32, std::string_view)> filter,
    c32 from,
    c32 to
) -> std::vector<c32> {
    UErrorCode ec{U_ZERO_ERROR};
    std::vector<c32> chars;
    struct Ctx {
        std::vector<c32>& chars; // Not the actual vector to support NRVO.
        decltype(filter) filter;
    } ctx{chars, std::move(filter)};

    auto Enum = []( // clang-format off
        void* context,
        UChar32 code,
        UCharNameChoice,
        const char* name,
        int32_t length
    ) -> UBool {
        auto& ctx = *static_cast<Ctx*>(context);
        if (ctx.filter(c32(code), std::string_view{name, usz(length)}))
            ctx.chars.push_back(c32(code));
        return true;
    }; // clang-format on

    u_enumCharNames(UChar32(from), UChar32(to), Enum, &ctx, U_UNICODE_CHAR_NAME, &ec);
    return chars;
}

auto smyth::unicode::Normalise(NormalisationForm form, UnicodeString str) -> Result<UnicodeString> {
    if (form == NormalisationForm::None) return str;
    auto GetTransliterator = [](const char* spec) -> Result<Transliterator*> {
        UErrorCode ec{U_ZERO_ERROR};
        auto norm = Transliterator::createInstance(
            spec,
            UTRANS_FORWARD,
            ec
        );

        if (U_FAILURE(ec)) return Err("Failed to create transliterator: {}\n", u_errorName(ec));
        return norm;
    };

    auto norm = Try(form == NormalisationForm::NFC ? GetTransliterator("NFC") : GetTransliterator("NFD"));
    norm->transliterate(str);
    return str;
}

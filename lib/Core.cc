#include <atomic>
#include <mutex>
#include <Smyth/Persistent.hh>
#include <Smyth/Unicode.hh>
#include <Smyth/Utils.hh>
#include <unicode/translit.h>
#include <thread>

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
template <typename Callback>
auto smyth::PersistentStore::ForEachEntry(
    DBRef db,
    std::string_view query,
    Callback cb
) -> Result<> {
    Try(Init(*db));
    auto stmt = Try(db->prepare(query));
    for (const auto& [key, entry] : entries) {
        defer { stmt.reset(); };
        stmt.bind(1, key);
        auto res = std::invoke(cb, stmt, entry.get());
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
    for (const auto& [key, entry] : entries)
        entry->reset();
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
auto smyth::Normalise(NormalisationForm form, icu::UnicodeString str) -> Result<icu::UnicodeString> {
    if (form == NormalisationForm::None) return str;
    auto GetTransliterator = [](const char* spec) -> Result<icu::Transliterator*> {
        UErrorCode ec{U_ZERO_ERROR};
        auto norm = icu::Transliterator::createInstance(
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

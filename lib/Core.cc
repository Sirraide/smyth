#include <atomic>
#include <Persistent.hh>
#include <Unicode.hh>
#include <unicode/translit.h>
#include <Utils.hh>

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
void smyth::PersistentStore::Init(Database& db) {
    static constexpr std::string_view query = R"sql(
        CREATE TABLE IF NOT EXISTS {} (
            key TEXT PRIMARY KEY,
            value BLOB
        ); -- *NOT* strict!!!
    )sql";

    auto err = db.exec(fmt::format(query, table_name));
    if (err.is_err()) throw Exception("Failed to initialise database: {}", err.err());
}

void smyth::PersistentStore::register_entry(std::string key, Entry entry) {
    if (entries.contains(key)) Fatal("Duplicate key '{}'", key);
    entries.emplace(std::move(key), std::move(entry));
}

void smyth::PersistentStore::reload_all(DBRef db) {
    static constexpr std::string_view query = R"sql(
        SELECT value FROM {} WHERE key = ?;
    )sql";

    Init(*db);
    auto stmt = db->prepare(fmt::format(query, table_name));
    if (stmt.is_err()) throw Exception("{}", stmt.err());
    for (const auto& [key, entry] : entries) {
        defer { stmt->reset(); };
        stmt->bind(1, key);
        auto res = stmt->fetch_optional();
        if (res.is_err()) throw Exception("Failed to load entry '{}': {}", key, res.err());
        if (not res->has_value()) continue;
        entry->load(Column(**res, 0));
    }
}

void smyth::PersistentStore::save_all(DBRef db) {
    static constexpr std::string_view query = R"sql(
        INSERT INTO {} (key, value) VALUES (?, ?)
        ON CONFLICT (key) DO UPDATE SET value = EXCLUDED.value;
    )sql";

    Init(*db);
    auto stmt = db->prepare(fmt::format(query, table_name));
    if (stmt.is_err()) throw Exception("{}", stmt.err());
    for (const auto& [key, entry] : entries) {
        stmt->bind(1, key);
        entry->save(QueryParamRef(*stmt, 2));
        auto res = stmt->exec();
        if (res.is_err()) throw Exception("Failed to save entry '{}': {}", key, res.err());
        stmt->reset();
    }
}

/// ====================================================================
///  Unicode
/// ====================================================================
icu::UnicodeString smyth::Normalise(NormalisationForm form, icu::UnicodeString str) {
    if (form == NormalisationForm::None) return str;
    auto GetTransliterator = [](const char* spec) {
        UErrorCode ec{U_ZERO_ERROR};
        auto norm = icu::Transliterator::createInstance(
            spec,
            UTRANS_FORWARD,
            ec
        );

        if (U_FAILURE(ec)) throw Exception("Failed to create transliterator: {}\n", u_errorName(ec));
        return norm;
    };

    auto norm = form == NormalisationForm::NFC ? GetTransliterator("NFC") : GetTransliterator("NFD");
    norm->transliterate(str);
    return str;
}

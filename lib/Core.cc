#include <atomic>
#include <Persistent.hh>
#include <Utils.hh>

/// ====================================================================
///  Error Handling
/// ====================================================================
namespace smyth {
namespace {
void DefaultHandler(std::string message) {
    fmt::print(stderr, "Error: {}\n", message);
}

std::atomic<ErrorMessageHandler*> handler = DefaultHandler;
}
} // namespace smyth

void smyth::detail::ErrorImpl(std::string message) {
    auto h = handler.load(std::memory_order_relaxed);
    if (h != nullptr) h(std::move(message));
}

void smyth::detail::FatalImpl(std::string message) {
    ErrorImpl(std::move(message));
    std::exit(1);
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
        );
    )sql";

    auto err = db.exec(fmt::format(query, table_name));
    if (err.is_err()) throw Exception("Failed to initialise database: {}", err.err());
}

void smyth::PersistentStore::register_entry(std::string key, Entry entry) {
    if (entries.contains(key)) Fatal("Duplicate key '{}'", key);
    entries.emplace(std::move(key), std::move(entry));
}

void smyth::PersistentStore::reload_all(Database& db) {
    static constexpr std::string_view query = R"sql(
        SELECT value FROM {} WHERE key = ?;
    )sql";

    auto stmt = db.prepare(fmt::format(query, table_name));
    if (stmt.is_err()) throw Exception("Failed to prepare statement: {}", stmt.err());
    for (const auto& [key, entry] : entries) {
        stmt->bind(0, key);
        auto res = stmt->fetch_one();
        if (res.is_err()) throw Exception("Failed to load entry '{}': {}", key, res.err());
        entry->load(res->text(0));
        stmt->reset();
    }
}

void smyth::PersistentStore::save_all(Database& db) {
    static constexpr std::string_view query = R"sql(
        INSERT INTO {} (key, value) VALUES (?, ?)
        ON CONFLICT (key) DO UPDATE SET value = EXCLUDED.value;
    )sql";

    auto stmt = db.prepare(fmt::format(query, table_name));
    if (stmt.is_err()) throw Exception("Failed to prepare statement: {}", stmt.err());
    for (const auto& [key, entry] : entries) {
        stmt->bind(0, key);
        stmt->bind(1, entry->save());
        auto res = stmt->exec();
        if (res.is_err()) throw Exception("Failed to save entry '{}': {}", key, res.err());
        stmt->reset();
    }
}

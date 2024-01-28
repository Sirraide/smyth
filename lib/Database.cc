#include <Database.hh>
#include <sqlite3.h>

smyth::Database::~Database() noexcept {
    sqlite3_close_v2(handle);
}

auto smyth::Database::BackupInternal(Database& to, Database& from) -> Result<> {
    sqlite3_backup* backup = sqlite3_backup_init(to.handle, "main", from.handle, "main");
    if (backup == nullptr) return Err("Failed to create backup: {}", from.last_error());

    auto res = sqlite3_backup_step(backup, -1);
    if (res != SQLITE_DONE) return Err("Failed to backup database: {}", from.last_error());

    sqlite3_backup_finish(backup);
    return {};
}

auto smyth::Database::CreateInMemory() -> std::shared_ptr<Database> {
    sqlite3* handle{};
    auto res = sqlite3_open_v2(
        ":memory:",
        &handle,
        SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE |
            SQLITE_OPEN_MEMORY |
            SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (res != SQLITE_OK) Fatal("Failed to open in-memory database: {}", sqlite3_errmsg(handle));
    return std::make_shared<Database>(handle, _make_shared_tag{});
}

auto smyth::Database::Load(std::string_view path) -> Result<std::shared_ptr<Database>> {
    auto db = Database::CreateInMemory();
    auto from = Try(Open(path, SQLITE_OPEN_READONLY));
    Try(BackupInternal(*db, *from));
    return db;
}

auto smyth::Database::Open(std::string_view path, int flags) -> Result<DBRef> {
    sqlite3* db;
    auto res = sqlite3_open_v2(
        path.data(),
        &db,
        flags | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (res != SQLITE_OK) return Err(
        "Failed to open database '{}': {}",
        path,
        sqlite3_errmsg(db)
    );
    return std::make_shared<Database>(db, _make_shared_tag{});
}

auto smyth::Database::backup(std::string_view path) -> Result<> {
    auto to = Try(Open(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
    return BackupInternal(*to, *this);
}

auto smyth::Database::exec(std::string_view query) -> Result<> {
    auto res = sqlite3_exec(handle, query.data(), nullptr, nullptr, nullptr);
    if (res != SQLITE_OK) return Err(
        "Failed to execute query {}\nCode: {}. Message: {}",
        query,
        res,
        sqlite3_errmsg(handle)
    );
    return {};
}

auto smyth::Database::last_error() -> std::string {
    return sqlite3_errmsg(handle);
}

auto smyth::Database::prepare(std::string_view query) -> Result<Statement> {
    sqlite3_stmt* stmt;
    auto res = sqlite3_prepare_v2(handle, query.data(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) return Err("Failed to prepare statement: {}", sqlite3_errmsg(handle));
    return Statement{shared_from_this(), stmt};
}

auto smyth::Row::blob(int index) -> std::vector<char> {
    auto data = reinterpret_cast<const char*>(sqlite3_column_blob(stmt, index));
    auto size = sqlite3_column_bytes(stmt, index);
    return {data, data + size};
}

auto smyth::Row::integer(int index) -> i64 {
    return sqlite3_column_int64(stmt, index);
}

auto smyth::Row::text(int index) -> std::string {
    auto data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, index));
    auto size = sqlite3_column_bytes(stmt, index);
    return {data, usz(size)};
}

smyth::Statement::~Statement() noexcept {
    sqlite3_finalize(stmt);
}

void smyth::Statement::bind(int index, std::string_view text) {
    auto res = sqlite3_bind_text(stmt, index, text.data(), int(text.size()), SQLITE_TRANSIENT);
    if (res != SQLITE_OK) Fatal("Failed to bind text: {}", handle->last_error());
}

void smyth::Statement::bind(int index, std::span<const std::byte> raw) {
    auto res = sqlite3_bind_blob(stmt, index, raw.data(), int(raw.size()), SQLITE_TRANSIENT);
    if (res != SQLITE_OK) Fatal("Failed to bind blob: {}", handle->last_error());
}

void smyth::Statement::bind(int index, i64 value) {
    auto res = sqlite3_bind_int64(stmt, index, value);
    if (res != SQLITE_OK) Fatal("Failed to bind integer: {}", handle->last_error());
}

auto smyth::Statement::exec() -> Result<> {
    auto res = sqlite3_step(stmt);
    if (res != SQLITE_DONE and res != SQLITE_ROW) return Err("Failed to execute statement: {}", handle->last_error());
    return {};
}

auto smyth::Statement::for_each(std::function<void(Row)> cb) -> Result<> {
    for (;;) {
        auto res = sqlite3_step(stmt);
        if (res == SQLITE_DONE) return {};
        if (res != SQLITE_ROW) return Err("Failed to execute statement: {}", handle->last_error());
        std::invoke(cb, Row{stmt});
    }
}

auto smyth::Statement::fetch_one() -> Result<Row> {
    auto res = Try(fetch_optional());
    if (not res.has_value()) return Err("Expected at least one entry!");
    return std::move(*res);
}

auto smyth::Statement::fetch_optional() -> Result<std::optional<Row>> {
    auto res = sqlite3_step(stmt);
    if (res == SQLITE_DONE) return std::optional<Row>{std::nullopt};
    if (res != SQLITE_ROW) return Err("Failed to execute statement: {}", handle->last_error());
    return std::optional{Row{stmt}};
}

void smyth::Statement::reset() {
    sqlite3_reset(stmt);
}

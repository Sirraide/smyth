#include <Database.hh>
#include <sqlite3.h>

smyth::Database::~Database() noexcept {
    sqlite3_close_v2(handle);
}

auto smyth::Database::BackupInternal(Database& to, Database& from) -> Res {
    sqlite3_backup* backup = sqlite3_backup_init(to.handle, "main", from.handle, "main");
    if (backup == nullptr) return fmt::format(
        "Failed to create backup: {}",
        from.last_error()
    );

    auto res = sqlite3_backup_step(backup, -1);
    if (res != SQLITE_DONE) return fmt::format(
        "Failed to backup database: {}",
        from.last_error()
    );

    sqlite3_backup_finish(backup);
    return {};
}

auto smyth::Database::Create() -> std::shared_ptr<Database> {
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

    if (res != SQLITE_OK) Fatal(
        "Failed to open database: {}",
        sqlite3_errmsg(handle)
    );

    return std::make_shared<Database>(handle, _make_shared_tag{});
}

auto smyth::Database::Load(std::string_view path) -> Result<std::shared_ptr<Database>, std::string> {
    auto db = Database::Create();
    auto from = Open(path, SQLITE_OPEN_READONLY);
    if (from.is_err()) return from.err();
    auto res = BackupInternal(*db, **from);
    if (res.is_err()) return res.err();
    return db;
}

auto smyth::Database::Open(std::string_view path, int flags) -> Result<DBRef, std::string> {
    sqlite3* db;
    auto res = sqlite3_open_v2(
        path.data(),
        &db,
        flags | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (res != SQLITE_OK) return fmt::format(
        "Failed to open database '{}': {}",
        path,
        sqlite3_errmsg(db)
    );
    return std::make_shared<Database>(db, _make_shared_tag{});
}

auto smyth::Database::backup(std::string_view path) -> Res {
    auto to = Open(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    if (to.is_err()) return to.err();
    return BackupInternal(**to, *this);
}

auto smyth::Database::exec(std::string_view query) -> Res {
    auto res = sqlite3_exec(handle, query.data(), nullptr, nullptr, nullptr);
    if (res != SQLITE_OK) return fmt::format(
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

auto smyth::Database::prepare(std::string_view query) -> Result<Statement, std::string> {
    sqlite3_stmt* stmt;
    auto res = sqlite3_prepare_v2(handle, query.data(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) return fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(handle));
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

void smyth::Statement::bind(int index, i64 value) {
    auto res = sqlite3_bind_int64(stmt, index, value);
    if (res != SQLITE_OK) Fatal("Failed to bind integer: {}", handle->last_error());
}

auto smyth::Statement::exec() -> Res {
    auto res = sqlite3_step(stmt);
    if (res != SQLITE_DONE and res != SQLITE_ROW) return fmt::format("Failed to execute statement: {}", handle->last_error());
    return {};
}

auto smyth::Statement::for_each(std::function<void(Row)> cb) -> Res {
    for (;;) {
        auto res = sqlite3_step(stmt);
        if (res == SQLITE_DONE) return {};
        if (res != SQLITE_ROW) return fmt::format("Failed to execute statement: {}", handle->last_error());
        std::invoke(cb, Row{stmt});
    }
}

auto smyth::Statement::fetch_one() -> Result<Row, std::string> {
    auto res = sqlite3_step(stmt);
    if (res == SQLITE_DONE) return std::string{"No rows"};
    if (res != SQLITE_ROW) return fmt::format("Failed to execute statement: {}", handle->last_error());
    return Row{stmt};
}

void smyth::Statement::reset() {
    sqlite3_reset(stmt);
}

#include <Database.hh>
#include <sqlite3.h>

smyth::Database::Database() {
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
}

smyth::Database::~Database() noexcept {
    sqlite3_close_v2(handle);
}

auto smyth::Database::backup(std::string_view path) -> Res {
    sqlite3* target;
    auto res = sqlite3_open_v2(
        path.data(),
        &target,
        SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE |
            SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (res != SQLITE_OK) return fmt::format(
        "Failed to open database '{}': {}",
        path,
        sqlite3_errmsg(target)
    );

    sqlite3_backup* backup = sqlite3_backup_init(target, "main", handle, "main");
    if (backup == nullptr) return fmt::format(
        "Failed to create backup: {}",
        sqlite3_errmsg(target)
    );

    res = sqlite3_backup_step(backup, -1);
    if (res != SQLITE_DONE) return fmt::format(
        "Failed to backup database: {}",
        sqlite3_errmsg(target)
    );

    sqlite3_backup_finish(backup);
    sqlite3_close_v2(target);
    return {};
}

auto smyth::Database::exec(std::string_view query) -> Res {
    char* err = nullptr;
    auto res = sqlite3_exec(handle, query.data(), nullptr, nullptr, &err);
    if (res != SQLITE_OK) return fmt::format("Failed to execute query {}\nMessage: {}", query, err);
    return {};
}

auto smyth::Database::prepare(std::string_view query) -> Result<Statement, std::string> {
    sqlite3_stmt* stmt;
    auto res = sqlite3_prepare_v2(handle, query.data(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) return fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(handle));
    return Statement{handle, stmt};
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
    auto res = sqlite3_bind_text(stmt, index, text.data(), -1, SQLITE_TRANSIENT);
    if (res != SQLITE_OK) Fatal("Failed to bind text: {}", sqlite3_errmsg(handle));
}

void smyth::Statement::bind(int index, i64 value) {
    auto res = sqlite3_bind_int64(stmt, index, value);
    if (res != SQLITE_OK) Fatal("Failed to bind integer: {}", sqlite3_errmsg(handle));
}

auto smyth::Statement::exec() -> Res {
    auto res = sqlite3_step(stmt);
    if (res != SQLITE_DONE and res != SQLITE_ROW) return fmt::format("Failed to execute statement: {}", sqlite3_errmsg(handle));
    return {};
}

auto smyth::Statement::for_each(std::function<void(Row)> cb) -> Res {
    for (;;) {
        auto res = sqlite3_step(stmt);
        if (res == SQLITE_DONE) return {};
        if (res != SQLITE_ROW) return fmt::format("Failed to execute statement: {}", sqlite3_errmsg(handle));
        std::invoke(cb, Row{stmt});
    }
}

auto smyth::Statement::fetch_one() -> Result<Row, std::string> {
    auto res = sqlite3_step(stmt);
    if (res == SQLITE_DONE) return std::string{"No rows"};
    if (res != SQLITE_ROW) return fmt::format("Failed to execute statement: {}", sqlite3_errmsg(handle));
    return Row{stmt};
}

void smyth::Statement::reset() {
    sqlite3_reset(stmt);
}

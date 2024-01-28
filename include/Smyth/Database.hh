#ifndef SMYTH_DATABASE_HH
#define SMYTH_DATABASE_HH

#include <functional>
#include <optional>
#include <Smyth/Result.hh>
#include <Smyth/Utils.hh>
#include <span>
#include <utility>

struct sqlite3;
struct sqlite3_stmt;

namespace smyth {
class Statement;
class Database;

using DBRef = std::shared_ptr<Database>;

class Database : public std::enable_shared_from_this<Database> {
    struct _make_shared_tag {};

    sqlite3* handle{};

    Database(sqlite3* handle) : handle(handle) {}

public:
    /// Internal. Do not use.
    Database(sqlite3* handle, _make_shared_tag) : Database(handle) {}

    /// Databases may be referenced by other objects, so we
    /// only pass them by shared_ptr and prevent them from
    /// being moved around.
    SMYTH_IMMOVABLE(Database);
    ~Database() noexcept;

    /// Save the DB to a file.
    auto backup(std::string_view path) -> Result<>;

    /// Execute an SQL query.
    auto exec(std::string_view query) -> Result<>;

    /// Get the last error.
    auto last_error() -> std::string;

    /// Prepare a statement.
    auto prepare(std::string_view query) -> Result<Statement>;

    /// Create a new database.
    static auto CreateInMemory() -> DBRef;

    /// Load a copy of a database from a file.
    static auto Load(std::string_view path) -> Result<DBRef>;

private:
    /// Helper to load from / save to a file.
    static auto BackupInternal(Database& to, Database& from) -> Result<>;

    /// Open a database file on disk.
    static auto Open(std::string_view path, int flags = 0) -> Result<DBRef>;
};

class Row {
    sqlite3_stmt* stmt{};

public:
    Row(sqlite3_stmt* stmt) : stmt(stmt) {}

    /// Get the value of a column.
    auto blob(int index) -> std::vector<char>;
    auto integer(int index) -> i64;
    auto text(int index) -> std::string;
};

class Column {
    Row row;
    int index{};

public:
    Column(Row row, int index) : row(row), index(index) {}

    auto blob() -> std::vector<char> { return row.blob(index); }
    auto integer() -> i64 { return row.integer(index); }
    auto text() -> std::string { return row.text(index); }
};

class Statement {
    DBRef handle{};
    sqlite3_stmt* stmt{};

    Statement(DBRef handle, sqlite3_stmt* stmt) : handle(handle), stmt(stmt) {}

public:
    friend Database;

    Statement(const Statement&) = delete;
    Statement(Statement&& other) noexcept
        : handle(std::exchange(other.handle, nullptr)),
          stmt(std::exchange(other.stmt, nullptr)) {}

    Statement& operator=(const Statement&) = delete;
    Statement& operator=(Statement&& other) noexcept {
        handle = std::exchange(other.handle, nullptr);
        stmt = std::exchange(other.stmt, nullptr);
        return *this;
    }

    ~Statement() noexcept;

    /// Bind a parameter.
    void bind(int index, std::string_view text);
    void bind(int index, std::span<const std::byte> raw);
    void bind(int index, i64 value);

    /// Bind any integer type.
    void bind(int index, std::integral auto value)
    requires (not std::is_same_v<decltype(value), i64>)
    {
        bind(index, static_cast<i64>(value));
    }

    /// Execute the statement.
    auto exec() -> Result<>;

    /// Execute the statement and call a callback for each row.
    auto for_each(std::function<void(Row)> cb) -> Result<>;

    /// Fetch one row from the database. Returns an error
    /// if there are no more rows.
    auto fetch_one() -> Result<Row>;

    /// Fetch up to one row from the database.
    auto fetch_optional() -> Result<std::optional<Row>>;

    /// Reset the statement.
    void reset();
};

class QueryParamRef {
    Statement* stmt;
    int index;

public:
    QueryParamRef(Statement& stmt, int index) : stmt(&stmt), index(index) {}

    void bind(std::string_view text) { stmt->bind(index, text); }
    void bind(std::span<const std::byte> raw) { stmt->bind(index, raw); }
    void bind(std::integral auto value) { stmt->bind(index, value); }
};

} // namespace smyth

#endif // SMYTH_DATABASE_HH

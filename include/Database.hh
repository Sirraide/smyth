#ifndef SMYTH_DATABASE_HH
#define SMYTH_DATABASE_HH

#include <functional>
#include <Result.hh>
#include <utility>
#include <Utils.hh>

struct sqlite3;
struct sqlite3_stmt;

namespace smyth {
class Statement;

class Database {
    sqlite3* handle{};

public:
    using Res = Result<void, std::string>;

    SMYTH_IMMOVABLE(Database);
    Database();
    ~Database() noexcept;

    /// Save the DB to a file.
    auto backup(std::string_view path) -> Res;

    /// Execute an SQL query.
    auto exec(std::string_view query) -> Res;

    /// Prepare a statement.
    auto prepare(std::string_view query) -> Result<Statement, std::string>;
};

class Row {
    sqlite3_stmt* stmt{};

public:
    Row(sqlite3_stmt* stmt) : stmt(stmt) {}

    /// Get the value of a column.
    auto integer(int index) -> i64;
    auto text(int index) -> std::string;
};

class Statement {
    sqlite3* handle{};
    sqlite3_stmt* stmt{};

    Statement(sqlite3* handle, sqlite3_stmt* stmt) : handle(handle), stmt(stmt) {}

public:
    friend Database;

    using Res = Result<void, std::string>;

    Statement(const Statement&) = delete;
    Statement(Statement&& other) noexcept :
        handle(std::exchange(other.handle, nullptr)),
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
    void bind(int index, i64 value);

    /// Execute the statement.
    auto exec() -> Res;

    /// Execute the statement and call a callback for each row.
    auto for_each(std::function<void(Row)> cb) -> Res;

    /// Fetch one row from the database. Returns an error
    /// if there are no more rows.
    auto fetch_one() -> Result<Row, std::string>;

    /// Reset the statement.
    void reset();
};

} // namespace smyth

#endif // SMYTH_DATABASE_HH

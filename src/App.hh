#ifndef SMYTH_APP_HH
#define SMYTH_APP_HH

#include <Lexurgy.hh>
#include <QObject>
#include <QUrl>
#include <Utils.hh>

struct sqlite3;

namespace smyth {
class Database {
    sqlite3* handle{};

public:
    using Res = Result<void, std::string>;

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    Database();
    ~Database() noexcept;

    /// Save the DB to a file.
    auto backup(std::string_view path) -> Res;

    /// Execute an SQL query.
    auto exec(std::string_view query) -> Res;
};

class App : public QObject {
    Q_OBJECT

    Lexurgy lexurgy;
    Database db;
    QString save_path;

public:
    App();

    /// Show an error to the user.
    template <typename... Args>
    static void ShowError(fmt::format_string<Args...> fmt, Args&&... args) {
        ShowError(QString::fromStdString(fmt::format(fmt, std::forward<Args>(args)...)));
    }

    /// Show an error to the user.
    static void ShowError(QString message);

    /// Show an error to the user and exit.
    template <typename... Args>
    [[noreturn]] static void ShowFatalError(fmt::format_string<Args...> fmt, Args&&... args) {
        ShowFatalError(QString::fromStdString(fmt::format(fmt, std::forward<Args>(args)...)));
    }

    /// Show an error to the user and exit.
    [[noreturn]] static void ShowFatalError(QString message);

signals:

public slots:
    /// Apply sound changes to the input string.
    auto applySoundChanges(QString inputs, QString sound_changes) -> QString;

    /// Save project to a file.
    void save();

    /// Save project to a file under a new name.
    void saveAs();

private:
    /// Save project to a file.
    ///
    /// This only handles the actual saving; everything else must
    /// be done before that.
    void SaveImpl();
};

} // namespace smyth

template <>
struct fmt::formatter<QString> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const QString& s, FormatContext& ctx) {
        return fmt::formatter<std::string>::format(s.toStdString(), ctx);
    }
};

#endif // SMYTH_APP_HH

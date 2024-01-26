#ifndef SMYTH_APP_HH
#define SMYTH_APP_HH

#include <Database.hh>
#include <Lexurgy.hh>
#include <Persistent.hh>
#include <QObject>
#include <QUrl>
#include <Utils.hh>

#define SMYTH_MAIN_STORE_KEY "store"

namespace smyth::detail {
template <typename Object, QString (Object::*Get)() const, void (Object::*Set)(const QString&)>
class PersistQString : public PersistentBase {
    Object* obj;

public:
    PersistQString(Object* obj) : obj(obj) {}

private:
    void load(std::string new_val) override {
        std::invoke(Set, obj, QString::fromStdString(new_val));
    }

    auto save() -> std::string override {
        return std::invoke(Get, obj).toStdString();
    }
};
} // namespace smyth::detail

namespace smyth {
class App : public QObject {
    Q_OBJECT

    Lexurgy lexurgy;
    Database db;
    QString save_path;

public:
    PersistentStore store{SMYTH_MAIN_STORE_KEY};

    SMYTH_IMMOVABLE(App);
    App() = default;

    /// Persist a QString property in the store.
    template <auto Get, auto Set, typename Object>
    void persist(std::string key, Object* obj) {
        using namespace detail;
        std::unique_ptr<PersistentBase> e{new PersistQString<Object, Get, Set>(obj)};
        store.register_entry(std::move(key), std::move(e));
    }

    /// Apply sound changes to the input string.
    auto applySoundChanges(QString inputs, QString sound_changes) -> QString;

    /// Open a project from a file.
    void open();

    /// Save project to a file.
    void save();

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

#ifndef SMYTH_APP_HH
#define SMYTH_APP_HH

#include <Database.hh>
#include <Lexurgy.hh>
#include <PersistObjects.hh>
#include <QObject>
#include <QUrl>
#include <Utils.hh>

#define SMYTH_MAIN_STORE_KEY "store"

namespace smyth {
namespace detail {
template <typename>
struct ExtractTypeImpl;

template <typename Type, typename Object>
struct ExtractTypeImpl<Type (Object::*)> {
    using type = Type;
};

template <typename Type, typename Object>
struct ExtractTypeImpl<Type (Object::*)() const> {
    using type = Type;
};

template <typename Type>
using ExtractType = typename ExtractTypeImpl<Type>::type;

}

class App : public QObject {
    Q_OBJECT

    Lexurgy lexurgy;
    DBRef db;
    QString save_path;

    std::mutex save_lock;

public:
    PersistentStore store{SMYTH_MAIN_STORE_KEY};

    SMYTH_IMMOVABLE(App);
    App() = default;

    /// Persist a QString property in the store.
    template <auto Get, auto Set, typename Object>
    void persist(std::string key, Object* obj) {
        using namespace detail;
        std::unique_ptr<PersistentBase> e{new PersistProperty<ExtractType<decltype(Get)>, Object, Get, Set>(obj)};
        store.register_entry(std::move(key), std::move(e));
    }

    /// Apply sound changes to the input string.
    auto apply_sound_changes(QString inputs, QString sound_changes) -> QString;

    /// Load the last project we had open.
    void load_last_open_project();

    /// Open a project from a file.
    void open();

    /// Save project to a file.
    void save();

private:
    /// Remember the last project we had open.
    void NoteLastOpenProject();

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

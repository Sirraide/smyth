#ifndef SMYTH_PERSISTENT_HH
#define SMYTH_PERSISTENT_HH

#include <Database.hh>
#include <unordered_map>

namespace smyth {
class PersistentStore;

namespace detail {
class PersistentBase {
protected:
    PersistentBase() = default;

public:
    SMYTH_IMMOVABLE(PersistentBase);
    virtual ~PersistentBase() = default;

private:
    friend PersistentStore;

    /// Initialise this entry from the DB.
    /// \throw std::runtime_error if there is an error during loading.
    virtual void load(Column c) = 0;

    /// Save this entry to the DB.
    /// \throw std::runtime_error if there is an error during saving.
    virtual void save(QueryParamRef param) = 0;
};
} // namespace detail

/// Persistent data store.
class PersistentStore {
public:
    using Entry = std::unique_ptr<detail::PersistentBase>;

private:
    std::string_view table_name;
    std::unordered_map<std::string, Entry> entries;

public:
    SMYTH_IMMOVABLE(PersistentStore);
    PersistentStore(std::string_view table_name) : table_name(table_name) {}

    /// \brief Register an entry to this store.
    ///
    /// \param key The key to use for this entry. If the key already
    ///            exists, the program is terminated. If there is a key
    ///            for this entry in the database, the load() function
    ///            of the entry will be called.
    ///
    /// \param entry The entry to register. When this store is saved
    ///              to the database, the save() function of this entry
    ///              will be called.
    void register_entry(std::string key, Entry entry);

    /// \brief Register several entries at once.
    ///
    /// \see register_entry().
    template <typename... Rest>
    void register_entries(std::string key, Entry e, Rest&&... rest) {
        register_entry(std::move(key), std::move(e));
        if constexpr (sizeof...(rest)) register_entries(std::forward<Rest>(rest)...);
    }

    /// Reload all entries from a database.
    /// \throw std::runtime_error if there is an error during loading.
    void reload_all(DBRef db);

    /// Save all entries to a database.
    /// \throw std::runtime_error if there is an error during saving.
    void save_all(DBRef db);

private:
    void Init(Database& db);
};
} // namespace smyth

#endif // SMYTH_PERSISTENT_HH

#ifndef SMYTH_PERSISTENT_HH
#define SMYTH_PERSISTENT_HH

#include <Smyth/Database.hh>
#include <Smyth/Result.hh>
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
    virtual auto load(Column c) -> Result<> = 0;

    /// Check if this entry has been modified and needs saving.
    virtual auto modified(Column c) -> Result<bool> = 0;

    /// Reset this entry to its default value.
    virtual void reset() = 0;

    /// Save this entry to the DB.
    virtual auto save(QueryParamRef param) -> Result<> = 0;
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

    /// Check if any of the entries need saving.
    auto modified(DBRef db) -> Result<bool>;

    /// Reload all entries from a database.
    auto reload_all(DBRef db) -> Result<>;

    /// Reset all entries to their default values.
    void reset_all();

    /// Save all entries to a database.
    auto save_all(DBRef db) -> Result<>;

private:
    template <typename Callback>
    auto ForEachEntry(DBRef db, std::string_view query, Callback cb) -> Result<>;

    auto Init(Database& db) -> Result<>;
};
} // namespace smyth

#endif // SMYTH_PERSISTENT_HH

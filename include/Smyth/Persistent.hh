#ifndef SMYTH_PERSISTENT_HH
#define SMYTH_PERSISTENT_HH

#include <Smyth/JSON.hh>
#include <Smyth/Utils.hh>
#include <unordered_map>
#include <unordered_set>

#define SMYTH_DECLARE_SERIALISER(in, out)                                  \
    namespace smyth::detail {                                              \
    template <>                                                            \
    struct Serialiser<out> {                                               \
        static auto Deserialise(const json_utils::json& j) -> Result<out>; \
        static auto Serialise(in val) -> json_utils::json;                 \
    };                                                                     \
    }

namespace smyth {
class PersistentStore;
}

namespace smyth::ui {
class App;
}

namespace smyth::detail {
inline constexpr usz DefaultPriority = ~0zu;
class PersistentBase;

template <typename>
struct ExtractTypeImpl;

template <typename Type, typename Object>
struct ExtractTypeImpl<Type(Object::*)> {
    using type = Type;
};

template <typename Type, typename Object>
struct ExtractTypeImpl<Type (Object::*)() const> {
    using type = Type;
};

template <typename Type>
using ExtractType = typename ExtractTypeImpl<Type>::type;

template <typename Ty>
struct Serialiser {
    static_assert(false, "No implementation of Serialiser<> for this type");
};

/// Helper to persist a ‘property’ of an object.
template <typename RawType, typename Object, auto Get, auto Set>
class PersistProperty;

/// Persist a property using a getter and setter in the store.
template <auto Get, auto Set, typename Object>
auto Persist(
    PersistentStore& store,
    std::string key,
    Object* obj,
    usz priority = DefaultPriority
) -> PersistentBase*;

template <typename I>
requires std::integral<I> or std::is_enum_v<I>
struct Serialiser<I>;
} // namespace smyth::detail

class smyth::detail::PersistentBase {
protected:
    PersistentBase() = default;

public:
    LIBBASE_IMMOVABLE(PersistentBase);
    virtual ~PersistentBase() = default;

private:
    friend PersistentStore;

    /// Initialise this entry from a save file.
    virtual auto load(const json_utils::json& j) -> Result<> = 0;

    /// Reset this entry to its default value.
    virtual void restore() = 0;

    /// Save this entry to a save file.
    virtual auto save() const -> Result<json_utils::json> = 0;
};

/// Helper to persist a ‘property’ of an object.
template <typename RawType, typename Object, auto Get, auto Set>
class smyth::detail::PersistProperty : public PersistentBase {
    using Type = std::decay_t<RawType>;
    Object* obj;
    Type default_value;

public:
    PersistProperty(Object* obj)
        : obj(obj),
          default_value(std::invoke(Get, obj)) {}

private:
    auto load(const json_utils::json& j) -> Result<> override {
        auto stored = Try(Serialiser<Type>::Deserialise(j));
        std::invoke(Set, obj, std::move(stored));
        return {};
    }

    void restore() override {
        std::invoke(Set, obj, default_value);
    }

    auto save() const -> Result<json_utils::json> override {
        return Serialiser<Type>::Serialise(std::invoke(Get, obj));
    }
};

/// Persistent data store.
class smyth::PersistentStore final : detail::PersistentBase {
public:
    struct Entry {
        std::unique_ptr<PersistentBase> entry;

        /// The priority of the entry; entries with higher priority
        /// are loaded first so as to ensure that the rest of the
        /// deserialisation process has access to the data.
        usz priority;

        Entry(std::unique_ptr<PersistentBase> entry, usz priority = detail::DefaultPriority)
            : entry(std::move(entry)), priority(priority) {}

        struct Eqv {
            auto operator()(std::string_view a, std::string_view b) const -> bool {
                return a == b;
            }
        };
    };

private:
    LIBBASE_IMMOVABLE(PersistentStore);
    friend ui::App;

    std::unordered_map<std::string, Entry, std::hash<std::string>, Entry::Eqv> entries;

    /// Only used by 'App'! Use App::CreateStore() instead.
    explicit PersistentStore() = default;
public:
    /// \brief Register an entry to this store.
    ///
    /// \param key The key to use for this entry. If the key already
    ///     exists, the program is terminated. If there is a key
    ///     for this entry in the database, the load() function
    ///     of the entry will be called.
    ///
    /// \param entry The entry to register. When this store is saved
    ///     to the database, the save() function of this entry will
    ///     be called.
    void register_entry(std::string key, Entry entry);

    /// \brief Register an entry with default priority.
    /// \see register_entry().
    void register_entry(std::string key, std::unique_ptr<PersistentBase> entry) {
        register_entry(std::move(key), {std::move(entry), detail::DefaultPriority});
    }

    /// \brief Register several entries at once.
    ///
    /// \see register_entry().
    template <typename... Rest>
    void register_entries(std::string key, Entry e, Rest&&... rest) {
        register_entry(std::move(key), std::move(e));
        if constexpr (sizeof...(rest)) register_entries(std::forward<Rest>(rest)...);
    }

    /// Reload all entries from a save file.
    auto reload_all(const json_utils::json& j) -> Result<>;

    /// Reset all entries to their default values.
    void reset_all();

    /// Save all entries to a save file.
    auto save_all() const -> Result<json_utils::json>;

private:
    auto load(const json_utils::json& j) -> Result<> override;
    void restore() override;
    auto save() const -> Result<json_utils::json> override;
    auto Entries();
};

/// Persist a property using a getter and setter in the store.
template <auto Get, auto Set, typename Object>
auto smyth::detail::Persist(
    PersistentStore& store,
    std::string key,
    Object* obj,
    base::usz priority
) -> PersistentBase* {
    using Property = PersistProperty<ExtractType<decltype(Get)>, Object, Get, Set>;
    std::unique_ptr<PersistentBase> e{new Property(obj)};
    auto ptr = e.get();
    store.register_entry(std::move(key), {std::move(e), priority});
    return ptr;
}

SMYTH_DECLARE_SERIALISER(std::string&&, std::string)
SMYTH_DECLARE_SERIALISER(i64, i64)
SMYTH_DECLARE_SERIALISER(u64, u64)
SMYTH_DECLARE_SERIALISER(bool, bool)

template <typename I>
requires std::integral<I> or std::is_enum_v<I>
struct smyth::detail::Serialiser<I> {
    static auto Deserialise(const json_utils::json& j) -> Result<I> {
        auto val = Try(Serialiser<i64>::Deserialise(j));
        if constexpr (std::unsigned_integral<I>) {
            if (val < 0) return Error("Negative value for unsigned integer: {}", val);
        } else {
            if (val < std::numeric_limits<I>::min()) return Error("Value too small: {}", val);
        }
        if (val > std::numeric_limits<I>::max()) return Error("Value too large: {}", val);
        return static_cast<I>(val);
    }

    static auto Serialise(I i) -> json_utils::json {
        return Serialiser<i64>::Serialise(i64(i));
    }
};

#endif // SMYTH_PERSISTENT_HH


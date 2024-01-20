#ifndef SMYTH_RESULT_HH
#define SMYTH_RESULT_HH

#include <Utils.hh>
#include <variant>

namespace smyth {
namespace rgs = std::ranges;
namespace vws = std::ranges::views;

/// Result type that can hold either a value or an error.
template <typename Type, typename Error, bool allow_construction_from_nullptr = not std::is_pointer_v<Type>>
requires (not std::is_reference_v<Type> and not std::is_reference_v<Error> and not std::is_same_v<Type, Error>)
class [[nodiscard]] Result {
public:
    using TypeArg = Type;
    using ValueType = std::conditional_t<std::is_void_v<Type>, std::monostate, Type>;
private:
    std::variant<ValueType, Error> data;

    /// Construct a null result.
    struct NullConstructTag {};
    Result(NullConstructTag)
    requires std::is_pointer_v<ValueType>
        : data(nullptr) {}

public:
    explicit Result()
    requires std::is_void_v<ValueType>
        : data(std::monostate{}) {}

    Result()
    requires (std::is_default_constructible_v<ValueType> and allow_construction_from_nullptr)
        : data(ValueType{}) {}

    /// Prohibit construction from nullptr unless explicitly allowed.
    Result(std::nullptr_t)
    requires (not allow_construction_from_nullptr)
    = delete;

    /// Create a result that holds a value.
    Result(ValueType value)
    requires (not std::is_void_v<Type>)
        : data(std::move(value)) {}

    /// Create a result that holds an error.
    Result(Error err) : data(std::move(err)) {}

    /// Create a result from another result.
    template <typename T, typename U>
    requires (not std::is_void_v<Type> and std::convertible_to<T, ValueType> and std::convertible_to<U, Error>)
    Result(Result<T, U>&& other) {
        if (other.is_err()) data = std::move(other.err());
        else data = std::move(other.value());
    }

    /// Check if the result holds a error.
    bool is_err() { return std::holds_alternative<Error>(data); }

    /// Check if the result holds a value.
    bool is_value() { return std::holds_alternative<ValueType>(data);}

    /// Get the value.
    [[nodiscard]] auto value() -> ValueType&
    requires (not std::is_void_v<Type>)
    { return std::get<ValueType>(data); }

    /// Get the error. Moved to simplify returning the error.
    [[nodiscard]] auto err() -> Error&&
    { return std::move(std::get<Error>(data)); }

    /// Check if this has a value.
    explicit operator bool() { return is_value(); }

    /// Access the underlying value.
    [[nodiscard]] auto operator*() -> ValueType& { return value(); }

    /// Access the underlying value.
    [[nodiscard]] auto operator->() -> ValueType
    requires std::is_pointer_v<ValueType>
    { return value(); }

    /// Access the underlying value.
    [[nodiscard]] auto operator->() -> ValueType*
    requires (not std::is_pointer_v<ValueType>)
    { return &value(); }

    /// Create an empty pointer result.
    ///
    /// This is an unsafe operation as there aren’t ever supposed
    /// to be empty results. Be careful when using this.
    static Result Null()
    requires std::is_pointer_v<ValueType>
    { return Result(NullConstructTag{}); }
};

} // namespace smyth
#endif // SMYTH_RESULT_HH

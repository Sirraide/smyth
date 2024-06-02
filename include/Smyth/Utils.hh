#ifndef FOOBAR_UTILS_HH
#define FOOBAR_UTILS_HH

#include <expected>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/std.h>
#include <libassert/assert.hpp>
#include <ranges>

#define SMYTH_STR_(X) #X
#define SMYTH_STR(X)  SMYTH_STR_(X)

#define SMYTH_CAT_(X, Y) X##Y
#define SMYTH_CAT(X, Y)  SMYTH_CAT_(X, Y)

#define SMYTH_IMMOVABLE(cls)             \
    cls(const cls&) = delete;            \
    cls& operator=(const cls&) = delete; \
    cls(cls&&) = delete;                 \
    cls& operator=(cls&&) = delete

#define LPAREN_ (
#define RPAREN_ )

/// Macro that propagates errors up the call stack.
///
/// The second optional argument to the macro, if present, should be an
/// expression that evaluates to a string that will be propagated up the
/// call stack as the error; the original error is in scope as `$`.
///
/// Example usage: Given
///
///     auto foo() -> Result<Bar> { ... }
///
/// we can write
///
///     Bar res = Try(foo());
///     Bar res = Try(foo(), std::format("Failed to do X: {}", $));
///
/// to invoke `foo` and propagate the error up the call stack, if there
/// is one; this way, we don’t have to actually write any verbose error
/// handling code.
///
/// (Yes, I know this macro is an abomination, but this is what happens
/// if you don’t have access to this as a language feature...)
#define Try(x, ...) ({                                                        \
    auto _res = x;                                                            \
    if (not _res) {                                                           \
        return std::unexpected(                                               \
            __VA_OPT__(                                                       \
                [&]([[maybe_unused]] std::string $) {                         \
            return __VA_ARGS__;                                               \
        }                                                                     \
            ) __VA_OPT__(LPAREN_) std::move(_res.error()) __VA_OPT__(RPAREN_) \
        );                                                                    \
    }                                                                         \
    using ResTy = std::remove_reference_t<decltype(*_res)>;                   \
    static_cast<typename ::smyth::detail::TryResultType<ResTy>::type>(*_res); \
})
// clang-format on

// My IDE doesn’t know about __builtin_expect_with_probability, for some reason.
#if !__has_builtin(__builtin_expect_with_probability)
#    define __builtin_expect_with_probability(x, y, z) __builtin_expect(x, y)
#endif

#define Assert(cond, ...)      LIBASSERT_ASSERT(cond __VA_OPT__(, fmt::format(__VA_ARGS__)))
#define DebugAssert(cond, ...) LIBASSERT_DEBUG_ASSERT(AK_DebugAssert, cond __VA_OPT__(, __VA_ARGS__))
#define Unreachable(...)       LIBASSERT_UNREACHABLE(__VA_OPT__(fmt::format(__VA_ARGS__)))
#define Todo(...)              Unreachable("Todo" __VA_OPT__(": " __VA_ARGS__))

#ifndef NDEBUG
#    define SMYTH_DEBUG(...) __VA_ARGS__
#else
#    define SMYTH_DEBUG(...)
#endif

#define defer [[maybe_unused]] ::smyth::detail::DeferImpl _ = [&]

namespace smyth {
using namespace std::literals;
namespace rgs = std::ranges;
namespace vws = std::ranges::views;
namespace fs = std::filesystem;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usz = size_t;
using uptr = uintptr_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using isz = ptrdiff_t;
using iptr = intptr_t;

using f32 = float;
using f64 = double;

namespace detail {
template <typename Callable>
class DeferImpl {
    Callable cb;
    SMYTH_IMMOVABLE(DeferImpl);

public:
    DeferImpl(Callable cb) : cb(cb) {}
    ~DeferImpl() { cb(); }
};

template <typename Ty>
struct TryResultType {
    using type = std::remove_reference_t<Ty>&&;
};

template <typename Ty>
requires std::is_void_v<Ty>
struct TryResultType<Ty> {
    using type = void;
};

template <typename Ty>
requires (not std::is_reference_v<Ty>)
class ReferenceWrapper {
    Ty* ptr;
public:
    ReferenceWrapper(Ty& ref) : ptr(&ref) {}
    operator Ty&() const { return *ptr; }
    auto operator&() const -> Ty* { return ptr; }
    auto operator->() const -> Ty* { return ptr; }
};

template <typename Ty>
concept Reference = std::is_reference_v<Ty>;

template <typename Ty>
concept NotReference = not Reference<Ty>;

template <typename Ty>
struct ResultImpl;

template <Reference Ty>
struct ResultImpl<Ty> {
    using type = std::expected<ReferenceWrapper<std::remove_reference_t<Ty>>, std::string>;
};

template <NotReference Ty>
struct ResultImpl<Ty> {
    using type = std::expected<Ty, std::string>;
};

template <typename Ty>
struct ResultImpl<std::reference_wrapper<Ty>> {
    using type = typename ResultImpl<Ty&>::type;
    static_assert(false, "Use Result<T&> instead of Result<reference_wrapper<T>>");
};

template <typename Ty>
struct ResultImpl<ReferenceWrapper<Ty>> {
    using type = typename ResultImpl<Ty&>::type;
    static_assert(false, "Use Result<T&> instead of Result<ReferenceWrapper<T>>");
};
} // namespace detail

template <typename T = void>
struct [[nodiscard]] Result : detail::ResultImpl<T>::type {
    using detail::ResultImpl<T>::type::type;
};

/// FIXME: Use 'std::format' everywhere now that it exists.
template <typename... Args>
[[nodiscard]] auto Error(fmt::format_string<Args...> fmt, Args&&... args) -> std::unexpected<std::string> {
    return std::unexpected(fmt::format(fmt, std::forward<Args>(args)...));
}

/// Convert an enum to the underlying integer type.
template <typename Ty>
requires std::is_enum_v<Ty>
[[nodiscard]] auto operator+(Ty ty) -> std::underlying_type_t<Ty> {
    return std::to_underlying(ty);
}

namespace utils {
using FileHandle = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

template <typename T, typename U>
concept is = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename... Funcs>
struct Overloaded : Funcs... {
    using Funcs::operator()...;
};

template <typename... Funcs>
Overloaded(Funcs...) -> Overloaded<Funcs...>;

/// std::visit, but with a better order of arguments.
template <typename Variant, typename Visitor>
constexpr decltype(auto) visit(Visitor&& visitor, Variant&& variant) {
    return std::visit(std::forward<Variant>(variant), std::forward<Visitor>(visitor));
}

/// Write a file to disk.
///
/// This checks whether the file exists, is a file, etc.
///
/// This does NOT prompt the user whether they want to override an existing
/// file etc. It only writes to disk. Use 'QFileDialog::getSaveFileName()'
/// for the former.
auto WriteFile(const fs::path& file_path, std::string_view contents) -> Result<>;
} // namespace utils

/// Print a debug message to the terminal.
template <typename... arguments>
void Debug(fmt::format_string<arguments...> fmt, arguments&&... args) {
    fmt::print(stderr, "\033[1;33m[Smyth] ");
    fmt::print(stderr, fmt, std::forward<arguments>(args)...);
    fmt::print(stderr, "\033[m\n");
}
} // namespace smyth

#endif // FOOBAR_UTILS_HH

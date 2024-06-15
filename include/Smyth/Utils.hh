#ifndef FOOBAR_UTILS_HH
#define FOOBAR_UTILS_HH

#include <algorithm>
#include <base/Base.hh>
#include <base/Macros.hh>
#include <ranges>

namespace smyth {
using namespace base;
}

/*#define SMYTH_STR_(X) #X
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

namespace detail {

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
} // namespace smyth*/

#endif // FOOBAR_UTILS_HH

#ifndef FOOBAR_UTILS_HH
#define FOOBAR_UTILS_HH

#include <fmt/format.h>
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

// clang-format off
#define SMYTH_ASSERT_IMPL(kind, cond, ...) (cond ? void(0) : \
    ::smyth::detail::AssertFail(                             \
        ::smyth::detail::AssertKind::kind,                   \
        #cond,                                               \
        __FILE__,                                            \
        __LINE__                                             \
        __VA_OPT__(, fmt::format(__VA_ARGS__))               \
    )                                                        \
)

#define SMYTH_ABORT_IMPL(kind, ...)             \
    ::smyth::detail::AssertFail(                \
        ::smyth::detail::AssertKind::kind,      \
        "",                                     \
        __FILE__,                               \
        __LINE__                                \
        __VA_OPT__(, fmt::format(__VA_ARGS__))  \
    )                                           \

#define Assert(cond, ...) SMYTH_ASSERT_IMPL(AK_Assert, cond __VA_OPT__(, __VA_ARGS__))
#define Todo(...) SMYTH_ABORT_IMPL(AK_Todo __VA_OPT__(, __VA_ARGS__))
#define Unreachable(...) SMYTH_ABORT_IMPL(AK_Unreachable __VA_OPT__(, __VA_ARGS__))
// clang-format on

#ifndef NDEBUG
#    define SMYTH_DEBUG(...)       __VA_ARGS__
#    define DebugAssert(cond, ...) SMYTH_ASSERT_IMPL(AK_DebugAssert, cond __VA_OPT__(, __VA_ARGS__))
#else
#    define SMYTH_DEBUG(...)
#    define DebugAssert(cond, ...)
#endif

#define defer [[maybe_unused]] ::smyth::detail::DeferImpl _ = [&]

namespace smyth {
using namespace std::literals;
namespace rgs = std::ranges;
namespace vws = std::ranges::views;

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

enum struct ErrorMessageType {
    Info,
    Error,
    Fatal,
};

namespace detail {
enum struct AssertKind {
    AK_Assert,
    AK_DebugAssert,
    AK_Todo,
    AK_Unreachable,
};

[[noreturn]] void AssertFail(
    AssertKind k,
    std::string_view condition,
    std::string_view file,
    int line,
    std::string&& message = ""
);

void MessageImpl(std::string message, ErrorMessageType type);

template <typename Callable>
class DeferImpl {
    Callable cb;
    SMYTH_IMMOVABLE(DeferImpl);

public:
    DeferImpl(Callable cb) : cb(cb) {}
    ~DeferImpl() { cb(); }
};

} // namespace detail

namespace utils {
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
} // namespace utils

using ErrorMessageHandler = void(std::string, ErrorMessageType type);

/// Display an informational message to the user.
template <typename... arguments>
void Info(fmt::format_string<arguments...> fmt, arguments&&... args) {
    detail::MessageImpl(fmt::format(fmt, std::forward<arguments>(args)...), ErrorMessageType::Info);
}

/// Display an error to the user.
template <typename... arguments>
void Error(fmt::format_string<arguments...> fmt, arguments&&... args) {
    detail::MessageImpl(fmt::format(fmt, std::forward<arguments>(args)...), ErrorMessageType::Error);
}

/// Display an error to the user and exit.
template <typename... arguments>
[[noreturn]] void Fatal(fmt::format_string<arguments...> fmt, arguments&&... args) {
    detail::MessageImpl(fmt::format(fmt, std::forward<arguments>(args)...), ErrorMessageType::Fatal);
    std::exit(1);
}

/// Register a message handler.
///
/// This handler will be invoked whenever an error is encountered. This
/// function is thread-safe.
///
/// \param handler The handler to register. If this is `nullptr`, the
///                default handler will be used.
void RegisterMessageHandler(ErrorMessageHandler handler);
} // namespace smyth

#endif // FOOBAR_UTILS_HH

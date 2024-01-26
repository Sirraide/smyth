#ifndef FOOBAR_UTILS_HH
#define FOOBAR_UTILS_HH

#include <fmt/format.h>

#define SMYTH_STR_(X) #X
#define SMYTH_STR(X)  SMYTH_STR_(X)

#define SMYTH_CAT_(X, Y) X##Y
#define SMYTH_CAT(X, Y)  SMYTH_CAT_(X, Y)

#define SMYTH_IMMOVABLE(cls)             \
    cls(const cls&) = delete;            \
    cls& operator=(const cls&) = delete; \
    cls(cls&&) = delete;                 \
    cls& operator=(cls&&) = delete

#define defer [[maybe_unused]] ::smyth::detail::DeferImpl _ = [&]

namespace smyth {
using namespace std::literals;

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

namespace detail {
void ErrorImpl(std::string);
[[noreturn]] void FatalImpl(std::string);

template <typename Callable>
class DeferImpl {
    Callable cb;
    SMYTH_IMMOVABLE(DeferImpl);

public:
    DeferImpl(Callable cb) : cb(cb) {}
    ~DeferImpl() { cb(); }
};
} // namespace detail

using ErrorMessageHandler = void(std::string);

struct Exception : std::runtime_error {
    template <typename... Args>
    Exception(fmt::format_string<Args...> fmt, Args&&... args)
        : std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...)) {}
};

/// Display an error to the user.
template <typename... arguments>
void Error(fmt::format_string<arguments...> fmt, arguments&&... args) {
    detail::ErrorImpl(fmt::format(fmt, std::forward<arguments>(args)...));
}

/// Display an error to the user and exit.
template <typename... arguments>
[[noreturn]] void Fatal(fmt::format_string<arguments...> fmt, arguments&&... args) {
    detail::FatalImpl(fmt::format(fmt, std::forward<arguments>(args)...));
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

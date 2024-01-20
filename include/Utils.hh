#ifndef FOOBAR_UTILS_HH
#define FOOBAR_UTILS_HH

#include <fmt/format.h>

#define SMYTH_STR_(X) #X
#define SMYTH_STR(X)  SMYTH_STR_(X)

#define SMYTH_CAT_(X, Y) X##Y
#define SMYTH_CAT(X, Y)  SMYTH_CAT_(X, Y)

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

template <typename... arguments>
[[noreturn]] void die(fmt::format_string<arguments...> fmt, arguments&&... args) {
    fmt::print(stderr, fmt, std::forward<arguments>(args)...);
    fmt::print(stderr, "\n");
    std::exit(1);
}
} // namespace smyth

#endif // FOOBAR_UTILS_HH

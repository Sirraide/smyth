#ifndef SMYTH_UNICODE_HH
#define SMYTH_UNICODE_HH

#include <Smyth/Result.hh>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

namespace smyth::unicode {
using namespace icu;

enum struct NormalisationForm {
    None, /// No normalisation.
    NFC,  /// Normalisation Form C.
    NFD,  /// Normalisation Form D.
};

/// Unicode character with helper functions.
struct c32 {
    char32_t value;

    constexpr c32() = default;
    constexpr c32(char32_t value) : value(value) {}
    explicit constexpr c32(std::integral auto value) : value(char32_t(value)) {}

    /// Get the category of a character.
    [[nodiscard]] auto category() const -> UCharCategory;

    /// Get the name of a character.
    [[nodiscard]] auto name() const -> Result<std::string>;

    /// Swap case.
    [[nodiscard]] auto swap_case() const -> c32;

    /// Convert to lower case.
    [[nodiscard]] auto to_lower() const -> c32;

    /// Convert to upper case.
    [[nodiscard]] auto to_upper() const -> c32;

    constexpr c32& operator++() {
        ++value;
        return *this;
    }

    constexpr c32 operator++(int) {
        auto tmp = *this;
        ++value;
        return tmp;
    }

    /// Convert to a char32_t.
    operator char32_t() const { return value; }
};

/// Highest valid unicode codepoint.
static constexpr c32 LastCodepoint = 0x10'FFFF;

/// Find all characters whose name contains one of the given strings.
///
/// If the query is empty, the result is unspecified.
[[nodiscard]] auto FindCharsByName(
    std::move_only_function<bool(c32, std::string_view)> filter,
    c32 from = 0,
    c32 to = LastCodepoint
) -> std::vector<c32>;

/// Convert a string to a normalised form.
[[nodiscard]] auto Normalise(
    NormalisationForm form,
    UnicodeString str
) -> Result<UnicodeString>;
} // namespace smyth::unicode

namespace smyth {
using unicode::c32;
}

template <>
struct fmt::formatter<smyth::unicode::c32> : formatter<smyth::u32> {
    template <typename FormatContext>
    auto format(const smyth::unicode::c32& c, FormatContext& ctx) {
        return formatter<smyth::u32>::format(smyth::u32(c.value), ctx);
    }
};

#endif // SMYTH_UNICODE_HH

#ifndef SMYTH_UNICODE_HH
#define SMYTH_UNICODE_HH

#include <Smyth/Result.hh>
#include <unicode/unistr.h>
#include <unicode/uchar.h>

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

    c32() = default;
    c32(char32_t value) : value(value) {}

    /// Get the category of a character.
    auto category() const -> UCharCategory;

    /// Get the name of a character.
    auto name() const -> Result<std::string>;

    /// Swap case.
    auto swap_case() const -> c32;

    /// Convert to lower case.
    auto to_lower() const -> c32;

    /// Convert to upper case.
    auto to_upper() const -> c32;

    /// Convert to a char32_t.
    operator char32_t() const { return value; }
};

/// Convert a string to a normalised form.
/// \throw Exception if there is a problem.
auto Normalise(NormalisationForm form, UnicodeString str) -> Result<UnicodeString>;
}

namespace smyth {
using unicode::c32;
}

template <>
struct fmt::formatter<smyth::unicode::c32> : fmt::formatter<smyth::u32> {
    template <typename FormatContext>
    auto format(const smyth::unicode::c32& c, FormatContext& ctx) {
        return fmt::formatter<smyth::u32>::format(smyth::u32(c.value), ctx);
    }
};


#endif // SMYTH_UNICODE_HH

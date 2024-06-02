#include <algorithm>
#include <functional>
#include <Smyth/Unicode.hh>
#include <Smyth/Utils.hh>
#include <unicode/translit.h>
#include <unicode/uchar.h>

/// ====================================================================
///  Unicode
/// ====================================================================
auto smyth::unicode::c32::category() const -> UCharCategory {
    return UCharCategory(u_charType(UChar32(value)));
}

auto smyth::unicode::c32::name() const -> Result<std::string> {
    UErrorCode ec{U_ZERO_ERROR};
    std::array<char, 1'024> char_name{};
    auto len = u_charName(UChar32(value), U_UNICODE_CHAR_NAME, char_name.data(), i32(char_name.size()), &ec);
    if (U_FAILURE(ec)) return Error("Failed to get name for codepoint: {}", u_errorName(ec));
    return std::string{char_name.data(), usz(len)};
}

auto smyth::unicode::c32::swap_case() const -> c32 {
    auto cat = category();
    if (cat == U_UPPERCASE_LETTER) return to_lower();
    if (cat == U_LOWERCASE_LETTER) return to_upper();
    return *this;
}

auto smyth::unicode::c32::to_lower() const -> c32 {
    return u_tolower(UChar32(value));
}

auto smyth::unicode::c32::to_upper() const -> c32 {
    return u_toupper(UChar32(value));
}

auto smyth::unicode::FindCharsByName(
    std::function<bool(c32, std::string_view)> filter,
    c32 from,
    c32 to
) -> std::vector<c32> {
    UErrorCode ec{U_ZERO_ERROR};
    std::vector<c32> chars;
    struct Ctx {
        std::vector<c32>& chars; // Not the actual vector to support NRVO.
        decltype(filter) filter;
    } ctx{chars, std::move(filter)};

    auto Enum = []( // clang-format off
        void* context,
        UChar32 code,
        UCharNameChoice,
        const char* name,
        int32_t length
    ) -> UBool {
        auto& ctx = *static_cast<Ctx*>(context);
        if (ctx.filter(c32(code), std::string_view{name, usz(length)}))
            ctx.chars.push_back(c32(code));
        return true;
    }; // clang-format on

    u_enumCharNames(UChar32(from), UChar32(to), Enum, &ctx, U_UNICODE_CHAR_NAME, &ec);
    return chars;
}

auto smyth::unicode::Normalise(NormalisationForm form, UnicodeString str) -> Result<UnicodeString> {
    if (form == NormalisationForm::None) return str;
    auto GetTransliterator = [](const char* spec) -> Result<Transliterator*> {
        UErrorCode ec{U_ZERO_ERROR};
        auto norm = Transliterator::createInstance(
            spec,
            UTRANS_FORWARD,
            ec
        );

        if (U_FAILURE(ec)) return Error("Failed to create transliterator: {}\n", u_errorName(ec));
        return norm;
    };

    auto norm = Try(form == NormalisationForm::NFC ? GetTransliterator("NFC") : GetTransliterator("NFD"));
    norm->transliterate(str);
    return str;
}

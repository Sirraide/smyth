#ifndef SMYTH_UNICODE_HH
#define SMYTH_UNICODE_HH

#include <unicode/unistr.h>

namespace smyth {
enum struct NormalisationForm {
    None, /// No normalisation.
    NFC,  /// Normalisation Form C.
    NFD,  /// Normalisation Form D.
};

/// Convert a string to a normalised form.
/// \throw Exception if there is a problem.
auto Normalise(NormalisationForm form, icu::UnicodeString str) -> icu::UnicodeString;

}

#endif // SMYTH_UNICODE_HH

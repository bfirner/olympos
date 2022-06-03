/*
 * Copyright 2022 Bernhard Firner
 *
 * Some utility functions.
 */

#include <cwchar>
#include <clocale>

#include "olympos_utility.hpp"

namespace OlymposUtility {
    std::wstring utf8ToWString(const std::string& input) {
        // Opting for this conversion over what is in codecvt due to some deprecations.
        // Otherwise we could do this:
        //   std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(input);
        std::mbstate_t state = std::mbstate_t();
        const char* in_data = input.c_str();
        // Find the number of characters for the sequence.
        std::size_t converted_len = mbsrtowcs(nullptr, &in_data, 0, &state);
        std::wstring converted(converted_len, L'\0');
        std::size_t written = mbsrtowcs(&converted[0], &in_data, converted.size(), &state);
        return converted;
    }
}

#include "unicode.h"

#include <algorithm>


template<typename char_t>
bool is_ascii(char_t const* const str, int const size)
{
	return std::all_of(str, str + size, [](char_t const e){ return e >= 32 && e <= 126; });
}

template bool is_ascii<char>(char const* const str, int const size);
template bool is_ascii<wchar_t>(wchar_t const* const str, int const size);

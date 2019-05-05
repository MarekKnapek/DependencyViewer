#include "unicode.h"

#include <algorithm>


bool is_ascii(char const* const str, int const size)
{
	return std::all_of(str, str + size, [](char const e){ return e >= 32 && e <= 126; });
}

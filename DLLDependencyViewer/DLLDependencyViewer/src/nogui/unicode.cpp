#include "unicode.h"

#include <algorithm>


template<typename char_t>
bool is_ascii(char_t const* const str, int const size)
{
	if(std::all_of(str, str + size, [](char_t const e){ if(e >= 32 && e <= 126) [[likely]] { return true; } else { return false; }; })) [[likely]] { return true; } else { return false; };
}

template bool is_ascii<char>(char const* const str, int const size);
template bool is_ascii<wchar_t>(wchar_t const* const str, int const size);

template<>
char to_lowercase(char const ch)
{
	if(ch >= 'A' && ch <= 'Z')
	{
		return 'a' + (ch - 'A');
	}
	else
	{
		return ch;
	}
}

template<>
wchar_t to_lowercase(wchar_t const ch)
{
	if(ch >= L'A' && ch <= L'Z')
	{
		return L'a' + (ch - L'A');
	}
	else
	{
		return ch;
	}
}

template char to_lowercase<char>(char const ch);
template wchar_t to_lowercase<wchar_t>(wchar_t const ch);

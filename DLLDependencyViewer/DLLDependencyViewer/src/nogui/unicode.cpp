#include "unicode.h"

#include <algorithm>
#include <cstdint>


template<typename T>
bool is_ascii_u(T const* const buff, int const size);


template<>
bool is_ascii(char const* const str, int const size)
{
	unsigned char const* const u = reinterpret_cast<unsigned char const*>(str);
	return is_ascii_u(u, size);
}

template<>
bool is_ascii(wchar_t const* const str, int const size)
{
	std::uint16_t const* const u = reinterpret_cast<std::uint16_t const*>(str);
	return is_ascii_u(u, size);
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


template<typename T>
bool is_ascii_u(T const* const buff, int const size)
{
	if(std::all_of(buff, buff + size, [](auto const& e) -> bool { if(e >= 32 && e <= 126) [[likely]] { return true; } else { return false; } })) [[likely]] { return true; } else { return false; }
}

#pragma once


#include "../nogui/unique_strings.h"

#include <array>
#include <string>


class string_converter
{
public:
	string_converter();
	wchar_t const* convert(string const* const str);
public:
	std::array<std::wstring, 4> m_strings;
	unsigned m_index;
};

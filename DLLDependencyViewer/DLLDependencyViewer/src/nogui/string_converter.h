#pragma once


#include "../nogui/my_string_handle.h"

#include <array>
#include <string>


class string_converter
{
public:
	string_converter();
	wchar_t const* convert(string_handle const& str);
public:
	std::array<std::wstring, 4> m_strings;
	unsigned m_index;
};

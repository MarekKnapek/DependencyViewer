#include "string_converter.h"

#include <algorithm>


string_converter::string_converter() :
	m_strings(),
	m_index()
{
}

wchar_t const* string_converter::convert(string_handle const& str)
{
	std::wstring& tmpstr = m_strings[m_index++ % m_strings.size()];
	tmpstr.resize(size(str));
	std::transform(cbegin(str), cend(str), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
	return tmpstr.c_str();
}

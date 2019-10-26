#include "string_converter.h"

#include <algorithm>


string_converter::string_converter() :
	m_strings(),
	m_index()
{
}

wchar_t const* string_converter::convert(string const* const str)
{
	std::wstring& tmpstr = m_strings[m_index++ % m_strings.size()];
	tmpstr.resize(str->m_len);
	std::transform(cbegin(str), cend(str), begin(tmpstr), [](char const& e) -> wchar_t { return static_cast<wchar_t>(e); });
	return tmpstr.c_str();
}

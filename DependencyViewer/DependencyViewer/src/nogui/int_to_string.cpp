#include "int_to_string.h"

#include "cassert_my.h"
#include "string_converter.h"

#include <array>
#include <cwchar>


wchar_t const* ordinal_to_string(std::uint16_t const ordinal, string_converter& converter)
{
	std::wstring& tmpstr = converter.m_strings[converter.m_index++ % converter.m_strings.size()];
	ordinal_to_string(ordinal, tmpstr);
	return tmpstr.c_str();
}

wchar_t const* rva_to_string(std::uint32_t const rva, string_converter& converter)
{
	std::wstring& tmpstr = converter.m_strings[converter.m_index++ % converter.m_strings.size()];
	rva_to_string(rva, tmpstr);
	return tmpstr.c_str();
}

void ordinal_to_string(std::uint16_t const ordinal, std::wstring& str)
{
	static_assert(sizeof(ordinal) == sizeof(unsigned short int), "");
	std::array<wchar_t, 15> buff;
	int const formatted = std::swprintf(buff.data(), buff.size(), L"%hu (0x%04hx)", static_cast<unsigned short int>(ordinal), static_cast<unsigned short int>(ordinal));
	assert(formatted >= 0);
	str.assign(buff.data(), buff.data() + formatted);
}

void rva_to_string(std::uint32_t const rva, std::wstring& str)
{
	static_assert(sizeof(rva) == sizeof(unsigned int), "");
	std::array<wchar_t, 11> buff;
	int const formatted = std::swprintf(buff.data(), buff.size(), L"0x%08x", static_cast<unsigned int>(rva));
	assert(formatted >= 0);
	str.assign(buff.data(), buff.data() + formatted);
}

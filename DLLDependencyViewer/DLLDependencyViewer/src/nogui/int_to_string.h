#pragma once


#include <cstdint>
#include <string>


class string_converter;


wchar_t const* ordinal_to_string(std::uint16_t const ordinal, string_converter& converter);
wchar_t const* rva_to_string(std::uint32_t const rva, string_converter& converter);

void ordinal_to_string(std::uint16_t const ordinal, std::wstring& str);
void rva_to_string(std::uint32_t const rva, std::wstring& str);

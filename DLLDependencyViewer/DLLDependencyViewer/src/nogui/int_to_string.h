#pragma once


#include <cstdint>
#include <string>


void ordinal_to_string(std::uint16_t const ordinal, std::wstring& str);
void rva_to_string(std::uint32_t const rva, std::wstring& str);

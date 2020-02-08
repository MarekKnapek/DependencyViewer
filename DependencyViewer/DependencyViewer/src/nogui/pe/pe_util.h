#pragma once


#include "coff_full.h"

#include <cstddef>
#include <cstdint>


struct pe_string
{
	char const* m_str;
	int m_len;
};


std::uint32_t pe_find_object_in_raw(std::byte const* const file_data, std::uint32_t const obj_va, std::uint32_t const obj_size, pe_section_header const*& sct);
bool pe_parse_string_rva(std::byte const* const file_data, std::uint32_t const str_rva, pe_string* const str_out);
bool pe_parse_string_raw(std::byte const* const file_data, std::uint32_t const str_raw, pe_section_header const& sct, pe_string* const str_out);
bool pe_is_ascii(char const* const& str, int const& len);

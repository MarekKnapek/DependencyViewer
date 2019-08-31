#pragma once


#include "coff_full.h"

#include <cstdint>


struct pe_string
{
	char const* m_str;
	int m_len;
};


std::uint32_t pe_find_object_in_raw(void const* const& file_data, int const& file_size, std::uint32_t const& obj_va, std::uint32_t const& obj_size, pe_section_header const*& sct);
bool pe_is_ascii(char const* const& str, int const& len);

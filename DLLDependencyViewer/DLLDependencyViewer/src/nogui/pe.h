#pragma once


#include <cstdint>


struct pe_header_info
{
	std::uint16_t m_pe_header_start;
	bool m_is_pe32;
	std::uint32_t m_data_directory_count;
	std::uint32_t m_section_count;
	std::uint32_t m_section_headers_start;
};


pe_header_info pe_get_coff_header(void const* const file_data, int const file_size);

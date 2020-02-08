#pragma once


#include <cstddef>
#include <cstdint>


enum class pe_e_parse_coff_header
{
	ok,
	file_too_small,
	file_ne,
	file_not_coff,
	unknown_machine_type,
	too_many_sections,
};


struct pe_coff_header
{
	std::uint32_t m_signature;
	std::uint16_t m_machine;
	std::uint16_t m_section_count;
	std::uint32_t m_date_time;
	std::uint32_t m_symbol_table;
	std::uint32_t m_symbol_table_entries_count;
	std::uint16_t m_optional_header_size;
	std::uint16_t m_characteristics;
};
static_assert(sizeof(pe_coff_header) == 24, "");
static_assert(sizeof(pe_coff_header) == 0x18, "");


pe_e_parse_coff_header pe_parse_coff_header(std::byte const* const file_data, int const file_size, pe_coff_header const** const header_out);

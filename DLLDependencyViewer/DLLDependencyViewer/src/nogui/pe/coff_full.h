#pragma once


#include "coff.h"
#include "coff_optional_standard.h"
#include "coff_optional_windows.h"

#include <cstddef>
#include <cstdint>


struct pe_coff_full_32
{
	pe_coff_header m_coff;
	pe_coff_optional_header_standard_32 m_standard;
	pe_coff_optional_header_windows_32 m_windows;
};
static_assert(sizeof(pe_coff_full_32) == 120, "");
static_assert(sizeof(pe_coff_full_32) == 0x78, "");

struct pe_coff_full_64
{
	pe_coff_header m_coff;
	pe_coff_optional_header_standard_64 m_standard;
	pe_coff_optional_header_windows_64 m_windows;
};
static_assert(sizeof(pe_coff_full_64) == 136, "");
static_assert(sizeof(pe_coff_full_64) == 0x88, "");

struct pe_coff_full_32_64
{
	union
	{
		pe_coff_full_32 m_32;
		pe_coff_full_64 m_64;
	};
};

struct pe_data_directory
{
	std::uint32_t m_va;
	std::uint32_t m_size;
};
static_assert(sizeof(pe_data_directory) == 8, "");
static_assert(sizeof(pe_data_directory) == 0x8, "");

struct pe_section_header
{
	std::uint8_t m_name[8];
	std::uint32_t m_virtual_size;
	std::uint32_t m_virtual_address;
	std::uint32_t m_raw_size;
	std::uint32_t m_raw_ptr;
	std::uint32_t m_relocations;
	std::uint32_t m_line_numbers;
	std::uint16_t m_relocation_count;
	std::uint16_t m_line_numbers_count;
	std::uint32_t m_characteristics;
};
static_assert(sizeof(pe_section_header) == 40, "");
static_assert(sizeof(pe_section_header) == 0x28, "");


enum class pe_e_directory_table
{
	export_table,
	import_table,
	resource_table,
	exception_table,
	certificate_table,
	base_relocation_table,
	debug,
	architecture,
	global_ptr,
	tls_table,
	load_config_table,
	bound_import,
	iat,
	delay_import_descriptor,
	clr_runtime_header,
	reserved,
};


bool pe_parse_coff_full_32_64(std::byte const* const file_data, int const file_size, pe_coff_full_32_64 const** const header_out);

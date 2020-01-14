#pragma once


#include <cstddef>
#include <cstdint>


struct pe_section_header;


struct pe_resource_directory_table
{
	std::uint32_t m_characteristics;
	std::uint32_t m_time_date_stamp;
	std::uint16_t m_major_version;
	std::uint16_t m_minor_version;
	std::uint16_t m_number_of_name_entries;
	std::uint16_t m_number_of_id_entries;
};
static_assert(sizeof(pe_resource_directory_table) == 16, "");
static_assert(sizeof(pe_resource_directory_table) == 0x10, "");

struct pe_resource_directory_entry
{
	union
	{
		std::uint32_t m_name_offset;
		std::uint32_t m_integer_id;
	};
	union
	{
		std::uint32_t m_data_entry_offset;
		std::uint32_t m_subdirectory_offset;
	};
};
static_assert(sizeof(pe_resource_directory_entry) == 8, "");
static_assert(sizeof(pe_resource_directory_entry) == 0x8, "");

struct pe_resource_directory_string
{
	std::uint16_t m_length;
	wchar_t m_unicode_string[1];
};
static_assert(sizeof(pe_resource_directory_string) == 4, "");
static_assert(sizeof(pe_resource_directory_string) == 0x4, "");

struct pe_resource_data_entry
{
	std::uint32_t m_data_rva;
	std::uint32_t m_size;
	std::uint32_t m_codepage;
	std::uint32_t m_reserved;
};
static_assert(sizeof(pe_resource_data_entry) == 16, "");
static_assert(sizeof(pe_resource_data_entry) == 0x10, "");


bool pe_parse_resource_root_directory_table(std::byte const* const file_data, pe_resource_directory_table const** const res_root_dir_tbl_out, pe_section_header const** const res_sct_out);
bool pe_parse_resource_directory_table(std::byte const* const file_data, pe_section_header const& res_sct, std::uint32_t const dir_tbl_offset, pe_resource_directory_table const** res_dir_tbl_out);
bool pe_parse_resource_directory_id_entry(pe_resource_directory_table const* const res_dir_tbl, std::uint16_t const idx, std::uint32_t* const entry_id_out);
bool pe_parse_resource_sub_directory_table(std::byte const* const file_data, pe_section_header const& res_sct, pe_resource_directory_table const* const res_dir_tbl, std::uint16_t const idx, pe_resource_directory_table const** const sub_dir_tbl_out);

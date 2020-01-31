#pragma once


#include <cstddef>
#include <cstdint>


struct pe_string;


struct  pe_export_directory_entry
{
	std::uint32_t m_characteristics;
	std::uint32_t m_date_time;
	std::uint16_t m_major;
	std::uint16_t m_minor;
	std::uint32_t m_name_rva;
	std::uint32_t m_ordinal_base;
	std::uint32_t m_export_address_count;
	std::uint32_t m_names_count;
	std::uint32_t m_export_address_table_rva;
	std::uint32_t m_export_name_table_rva;
	std::uint32_t m_ordinal_table_rva;
};
static_assert(sizeof(pe_export_directory_entry) == 40, "");
static_assert(sizeof(pe_export_directory_entry) == 0x28, "");

struct pe_export_directory_table
{
	pe_export_directory_entry const* m_table; // Single row.
};


struct pe_export_address_entry
{
	std::uint32_t m_export_rva;
};
static_assert(sizeof(pe_export_address_entry) == 4, "");
static_assert(sizeof(pe_export_address_entry) == 0x4, "");

struct pe_export_address_table
{
	pe_export_address_entry const* m_table;
	std::uint16_t m_count;
};


struct pe_export_name_pointer_entry
{
	std::uint32_t m_export_address_name_rva;
};
static_assert(sizeof(pe_export_name_pointer_entry) == 4, "");
static_assert(sizeof(pe_export_name_pointer_entry) == 0x4, "");

struct pe_export_name_pointer_table
{
	pe_export_name_pointer_entry const* m_table;
	std::uint16_t m_count;
};


struct pe_export_ordinal_entry
{
	std::uint16_t m_idx_to_eat;
};
static_assert(sizeof(pe_export_ordinal_entry) == 2, "");
static_assert(sizeof(pe_export_ordinal_entry) == 0x2, "");
bool operator==(pe_export_ordinal_entry const& a, pe_export_ordinal_entry const& b);

struct pe_export_ordinal_table
{
	pe_export_ordinal_entry const* m_table;
	std::uint16_t m_count;
};


bool pe_parse_export_directory_table(std::byte const* const file_data, pe_export_directory_table* const edt_out);
bool pe_parse_export_name_pointer_table(std::byte const* const file_data, pe_export_directory_table const& edt, pe_export_name_pointer_table* const enpt_out);
bool pe_parse_export_ordinal_table(std::byte const* const file_data, pe_export_directory_table const& edt, pe_export_ordinal_table* const eot_out);
bool pe_parse_export_address_table(std::byte const* const file_data, pe_export_directory_table const& edt, pe_export_address_table* const eat_out);
bool pe_parse_export_address_name(std::byte const* const file_data, pe_export_name_pointer_table const& enpt, pe_export_ordinal_table const& eot, std::uint16_t const& idx, std::uint16_t* const hint_out, pe_string* const ean_out);

#pragma once


#include <cstdint>


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
	union export_or_forwarder_rva
	{
		std::uint32_t m_export_rva;
		std::uint32_t m_forwarder_rva;
	};
	export_or_forwarder_rva m_export_or_forwarder_rva;
};
static_assert(sizeof(pe_export_address_entry) == 4, "");
static_assert(sizeof(pe_export_address_entry) == 0x4, "");

struct pe_export_address_table
{
	pe_export_address_entry const* m_table;
	int m_count;
};


struct pe_export_name_pointer_entry
{
	std::uint32_t m_ptr_to_export_name_table;
};
static_assert(sizeof(pe_export_name_pointer_entry) == 4, "");
static_assert(sizeof(pe_export_name_pointer_entry) == 0x4, "");

struct pe_export_name_pointer_table
{
	pe_export_name_pointer_entry const* m_table;
	int m_count;
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
	int m_count;
};


struct pe_export_name_entry
{
	std::uint32_t m_string_rva;
};
static_assert(sizeof(pe_export_name_entry) == 4, "");
static_assert(sizeof(pe_export_name_entry) == 0x4, "");

struct pe_export_name_table
{
	pe_export_name_entry const* m_table;
	int m_count;
};


bool pe_parse_export_directory_table(void const* const file_data, int const file_size, pe_export_directory_table& edt_out);
bool pe_parse_export_name_pointer_table(void const* const file_data, int const file_size, pe_export_directory_table const& edt, pe_export_name_pointer_table& enpt_out);
bool pe_parse_export_ordinal_table(void const* const file_data, int const file_size, pe_export_directory_table const& edt, pe_export_ordinal_table& eot_out);

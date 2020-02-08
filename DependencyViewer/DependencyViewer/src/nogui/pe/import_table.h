#pragma once


#include "pe_util.h"

#include <cstddef>
#include <cstdint>


struct pe_import_directory_entry
{
	std::uint32_t m_import_lookup_table;
	std::uint32_t m_date_time;
	std::uint32_t m_forwarder_chain;
	std::uint32_t m_name;
	std::uint32_t m_import_adress_table;
};
static_assert(sizeof(pe_import_directory_entry) == 20, "");
static_assert(sizeof(pe_import_directory_entry) == 0x14, "");
bool operator==(pe_import_directory_entry const& a, pe_import_directory_entry const& b);

struct pe_import_directory_table
{
	pe_import_directory_entry const* m_table;
	std::uint16_t m_count;
};

struct pe_import_address_table
{
	std::uint32_t m_raw;
	std::uint16_t m_count;
};

struct pe_import_lookup_entry_32
{
	std::uint32_t m_value;
};
static_assert(sizeof(pe_import_lookup_entry_32) == 4, "");
static_assert(sizeof(pe_import_lookup_entry_32) == 0x4, "");
bool operator==(pe_import_lookup_entry_32 const& a, pe_import_lookup_entry_32 const& b);

struct pe_import_lookup_entry_64
{
	std::uint64_t m_value;
};
static_assert(sizeof(pe_import_lookup_entry_64) == 8, "");
static_assert(sizeof(pe_import_lookup_entry_64) == 0x8, "");
bool operator==(pe_import_lookup_entry_64 const& a, pe_import_lookup_entry_64 const& b);

struct pe_hint_name
{
	std::uint16_t m_hint;
	pe_string m_name;
};

struct pe_delay_load_descriptor
{
	std::uint32_t m_attributes;
	std::uint32_t m_dll_name_rva;
	std::uint32_t m_module_handle_rva;
	std::uint32_t m_import_address_table_rva;
	std::uint32_t m_import_name_table_rva;
	std::uint32_t m_bound_import_address_table_rva;
	std::uint32_t m_unload_information_table_rva;
	std::uint32_t m_time_date_stamp;
};
static_assert(sizeof(pe_delay_load_descriptor) == 32, "");
static_assert(sizeof(pe_delay_load_descriptor) == 0x20, "");
bool operator==(pe_delay_load_descriptor const& a, pe_delay_load_descriptor const& b);

struct pe_delay_import_table
{
	pe_delay_load_descriptor const* m_table;
	std::uint16_t m_count;
};

struct pe_delay_load_import_address_table
{
	std::uint32_t m_raw;
	std::uint16_t m_count;
};


bool pe_parse_import_table(std::byte const* const file_data, pe_import_directory_table* const idt_out);
bool pe_parse_import_dll_name(std::byte const* const file_data, pe_import_directory_entry const& ide, pe_string* const dll_name_out);
bool pe_parse_import_address_table(std::byte const* const file_data, pe_import_directory_entry const& ide, pe_import_address_table* const iat_out);
bool pe_parse_import_address(std::byte const* const file_data, pe_import_address_table const& iat_in, int const& idx, bool* const is_ordinal_out, std::uint16_t* const ordinal_out, pe_hint_name* const hint_name_out);

bool pe_parse_delay_import_table(std::byte const* const file_data, pe_delay_import_table* const dlit_out);
bool pe_parse_delay_import_dll_name(std::byte const* const file_data, pe_delay_load_descriptor const& dld, pe_string* const dll_name_out);
bool pe_parse_delay_import_address_table(std::byte const* const file_data, pe_delay_load_descriptor const& dld, pe_delay_load_import_address_table* const dliat_out);
bool pe_parse_delay_import_address(std::byte const* const file_data, pe_delay_load_descriptor const& dld, pe_delay_load_import_address_table const& dliat_in, int const& idx, bool* const is_ordinal_out, std::uint16_t* const ordinal_out, pe_hint_name* const hint_name_out);

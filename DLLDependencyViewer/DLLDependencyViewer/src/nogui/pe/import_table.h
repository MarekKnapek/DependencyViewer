#pragma once


#include "pe_util.h"

#include <cstdint>


struct import_directory_entry
{
	std::uint32_t m_import_lookup_table;
	std::uint32_t m_date_time;
	std::uint32_t m_forwarder_chain;
	std::uint32_t m_name;
	std::uint32_t m_import_adress_table;
};
static_assert(sizeof(import_directory_entry) == 20, "");
static_assert(sizeof(import_directory_entry) == 0x14, "");

struct import_directory_table
{
	import_directory_entry const* m_table;
	int m_size;
	section_header const* m_sct;
};


struct import_lookup_entry_32
{
	std::uint32_t m_value;
};
static_assert(sizeof(import_lookup_entry_32) == 4, "");
static_assert(sizeof(import_lookup_entry_32) == 0x4, "");

struct import_lookup_entry_64
{
	std::uint64_t m_value;
};
static_assert(sizeof(import_lookup_entry_64) == 8, "");
static_assert(sizeof(import_lookup_entry_64) == 0x8, "");

struct import_lookup_table_32
{
	import_lookup_entry_32 const* m_table;
	int m_size;
	section_header const* m_sct;
};

struct import_lookup_table_64
{
	import_lookup_entry_64 const* m_table;
	int m_size;
	section_header const* m_sct;
};

struct hint_name
{
	std::uint16_t m_hint;
	pe_string m_name;
};


bool pe_parse_import_directory_table(void const* const& file_data, int const& file_size, import_directory_table& idt);
bool pe_parse_import_dll_name(void const* const& file_data, int const& file_size, import_directory_table const& idt, int const& idx, pe_string& str);
bool pe_parse_import_lookup_table_32(void const* const& file_data, int const& file_size, import_directory_table const& idt, int const& idx, import_lookup_table_32& ilt);
bool pe_parse_import_lookup_table_64(void const* const& file_data, int const& file_size, import_directory_table const& idt, int const& idx, import_lookup_table_64& ilt);
bool pe_parse_import_lookup_entry_32(void const* const& file_data, int const& file_size, import_lookup_table_32 const& ilt, int const& idx, bool& is_ordinal);
bool pe_parse_import_lookup_entry_64(void const* const& file_data, int const& file_size, import_lookup_table_64 const& ilt, int const& idx, bool& is_ordinal);
bool pe_parse_import_lookup_entry_ordinal_32(void const* const& file_data, int const& file_size, import_lookup_table_32 const& ilt, int const& idx, std::uint16_t& ordinal);
bool pe_parse_import_lookup_entry_ordinal_64(void const* const& file_data, int const& file_size, import_lookup_table_64 const& ilt, int const& idx, std::uint16_t& ordinal);
bool pe_parse_import_lookup_entry_hint_name_32(void const* const& file_data, int const& file_size, import_lookup_table_32 const& ilt, int const& idx, hint_name& hntnm);
bool pe_parse_import_lookup_entry_hint_name_64(void const* const& file_data, int const& file_size, import_lookup_table_64 const& ilt, int const& idx, hint_name& hntnm);
bool pe_parse_import_lookup_entry_hint_name(void const* const& file_data, int const& file_size, section_header const& sct, std::uint32_t const& hint_name_rva, hint_name& hntnm);

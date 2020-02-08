#pragma once


#include "array_bool.h"
#include "my_string_handle.h"

#include <cstdint>


struct pe_import_table_info
{
	std::uint16_t m_normal_dll_count;
	std::uint16_t m_delay_dll_count;
	string_handle const* m_dll_names;
	std::uint16_t const* m_import_counts;
	array_bool const* m_are_ordinals;
	std::uint16_t const* const* m_ordinals_or_hints;
	string_handle const* const* m_names;
	string_handle* const* m_undecorated_names;
	std::uint16_t* const* m_matched_exports;
};

union pe_rva_or_forwarder
{
	std::uint32_t m_rva;
	string_handle m_forwarder;
};

struct pe_export_table_info
{
	std::uint16_t m_count;
	std::uint16_t m_ordinal_base;
	std::uint16_t const* m_ordinals;
	array_bool m_are_rvas;
	pe_rva_or_forwarder const* m_rvas_or_forwarders;
	std::uint16_t const* m_hints;
	string_handle* m_names;
	string_handle* m_undecorated_names;
	array_bool m_are_used;
};

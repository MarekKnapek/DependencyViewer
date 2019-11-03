#pragma once


#include "my_vector.h"
#include "my_string_handle.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>


struct section_header;
class memory_manager;


struct pe_header_info
{
	std::byte const* m_file_data;
	std::uint32_t m_file_size;
	std::uint16_t m_pe_header_start;
	bool m_is_pe32;
	std::uint64_t m_image_base;
	std::uint32_t m_data_directory_count;
	std::uint32_t m_data_directory_start;
	std::uint32_t m_section_count;
	std::uint32_t m_section_headers_start;
};

struct pe_import_table_info
{
	std::uint16_t m_dll_count;
	std::uint16_t m_non_delay_dll_count;
	string_handle const* m_dll_names;
	std::uint16_t const* m_import_counts;
	unsigned const* const* m_are_ordinals;
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
	unsigned const* m_are_rvas;
	pe_rva_or_forwarder const* m_rvas_or_forwarders;
	std::uint16_t const* m_hints;
	string_handle* m_names;
	string_handle* m_undecorated_names;
	unsigned* m_are_used;
};

struct pe_resource_string_or_id
{
	bool m_is_string;
	union
	{
		wstring_handle m_string;
		std::uint16_t m_id;
	};
};

struct pe_resource
{
	pe_resource_string_or_id m_type;
	pe_resource_string_or_id m_name;
	pe_resource_string_or_id m_lang;
	std::byte const* m_data;
	std::uint32_t m_size;
	std::uint32_t m_code_page;
};

struct pe_resources_table_info
{
	std::vector<pe_resource> m_resources;
};


pe_header_info pe_process_header(std::byte const* const file_data, int const file_size);
pe_resources_table_info pe_process_resource_table(std::byte const* const file_data, int const file_size, pe_header_info const& hi, memory_manager& mm);

std::pair<section_header const*, std::uint32_t> convert_rva_to_disk_ptr(std::uint32_t const rva, pe_header_info const& hi, section_header const* const section = nullptr);

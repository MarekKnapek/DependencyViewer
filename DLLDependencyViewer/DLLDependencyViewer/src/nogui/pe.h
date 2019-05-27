#pragma once


#include <cstdint>
#include <string>
#include <utility>
#include <vector>


struct section_header;
class memory_manager;
template<typename> struct basic_string; typedef basic_string<char> string;

struct pe_header_info
{
	char const* m_file_data;
	std::uint32_t m_file_size;
	std::uint16_t m_pe_header_start;
	bool m_is_pe32;
	std::uint64_t m_image_base;
	std::uint32_t m_data_directory_count;
	std::uint32_t m_data_directory_start;
	std::uint32_t m_section_count;
	std::uint32_t m_section_headers_start;
};

struct pe_import_entry
{
	bool m_is_ordinal;
	std::uint16_t m_ordinal_or_hint;
	string const* m_name;
};

struct pe_import_dll_with_entries
{
	string const* m_dll_name;
	std::vector<pe_import_entry> m_entries;
};

struct pe_import_table_info
{
	std::vector<pe_import_dll_with_entries> m_dlls;
	int m_nondelay_imports_count;
};

struct pe_export_address_entry
{
	bool m_is_rva;
	std::uint32_t m_ordinal;
	std::uint32_t m_rva;
	string const* m_forwarder;
	std::uint16_t m_hint;
	string const* m_name;
};

struct pe_export_table_info
{
	std::vector<pe_export_address_entry> m_export_address_table;
};


pe_header_info pe_process_header(void const* const file_data, int const file_size);
pe_import_table_info pe_process_import_table(void const* const file_data, int const file_size, pe_header_info const& hi, memory_manager& mm);
pe_export_table_info pe_process_export_table(void const* const file_data, int const file_size, pe_header_info const& hi, memory_manager& mm);

std::pair<section_header const*, std::uint32_t> convert_rva_to_disk_ptr(std::uint32_t const rva, pe_header_info const& hi, section_header const* const section = nullptr);

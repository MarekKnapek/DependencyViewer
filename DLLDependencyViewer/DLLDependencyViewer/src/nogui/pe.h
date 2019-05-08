#pragma once


#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <tuple>


struct section_header;


struct pe_header_info
{
	char const* m_file_data;
	std::uint32_t m_file_size;
	std::uint16_t m_pe_header_start;
	bool m_is_pe32;
	std::uint32_t m_data_directory_count;
	std::uint32_t m_data_directory_start;
	std::uint32_t m_section_count;
	std::uint32_t m_section_headers_start;
};

struct pe_import_dll_with_entries
{
	std::string m_dll;
	std::vector<std::tuple<bool, std::uint16_t, std::string>> m_entries;
};

struct pe_import_table_info
{
	std::vector<pe_import_dll_with_entries> m_dlls;
};


pe_header_info pe_process_header(void const* const file_data, int const file_size);
pe_import_table_info pe_process_import_table(void const* const file_data, int const file_size, pe_header_info const& hi);

std::pair<section_header const*, std::uint32_t> convert_rva_to_disk_ptr(std::uint32_t const rva, pe_header_info const& hi, section_header const* const section = nullptr);

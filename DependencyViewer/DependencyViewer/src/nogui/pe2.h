#pragma once


#include "allocator.h"
#include "memory_manager.h"
#include "pe.h"
#include "unique_strings.h"

#include "pe/coff_full.h"
#include "pe/export_table.h"
#include "pe/import_table.h"
#include "pe/mz.h"

#include <cstddef>


struct pe_headers
{
	pe_dos_header const* m_dos;
	pe_coff_full_32_64 const* m_coff;
};

struct pe_import_tables
{
	pe_import_directory_table m_idt;
	pe_delay_import_table m_didt;
};

struct pe_import_names
{
	pe_import_tables const* m_tables;
	unique_strings* m_ustrings;
	allocator* m_alc;
	string_handle const* m_names_out;
};

struct pe_import_iat
{
	pe_headers* m_headers;
	pe_import_tables const* m_tables;
	unique_strings* m_ustrings;
	allocator* m_alc;
	pe_import_table_info* m_iti_out;
};

struct pe_export_eat
{
	pe_headers* m_headers;
	unique_strings* m_ustrings;
	allocator* m_alc;
	allocator* m_tmp_alc;
	pe_export_table_info* m_eti_out;
	std::uint16_t* m_enpt_count_out;
	std::uint16_t const** m_enpt_out;
};

struct pe_tables
{
	allocator* m_tmp_alc;
	pe_import_table_info* m_iti_out;
	pe_export_table_info* m_eti_out;
	std::uint16_t* m_enpt_count_out;
	std::uint16_t const** m_enpt_out;
	std::uint32_t m_manifest_id;
	bool m_is_32_bit;
};


bool pe_process_headers(std::byte const* const file_data, int const file_size, pe_headers* const headers_out);

bool pe_process_import_tables(std::byte const* const file_data, pe_import_tables* const tables_out);
bool pe_process_import_names(std::byte const* const file_data, pe_import_names* const names_in_out);
bool pe_process_import_iat(std::byte const* const file_data, pe_import_iat* const iat_in_out);

bool pe_process_export_eat(std::byte const* const file_data, pe_export_eat* const eat_in_out);

bool pe_process_resource_manifest(std::byte const* const file_data, bool const is_dll, std::uint32_t* const manifest_id_out);

bool pe_process_all(std::byte const* const file_data, int const file_size, memory_manager& mm, pe_tables* const tables_in_out);

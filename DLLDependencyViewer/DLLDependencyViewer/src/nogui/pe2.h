#pragma once


#include "allocator.h"
#include "memory_manager.h"
#include "pe.h"
#include "unique_strings.h"

#include "pe/coff_full.h"
#include "pe/import_table.h"
#include "pe/mz.h"


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
	string const* const* m_names_out;
};

struct pe_import_iat
{
	pe_headers* m_headers;
	pe_import_tables const* m_tables;
	unique_strings* m_ustrings;
	allocator* m_alc;
	pe_import_table_info* m_iti_out;
};


bool pe_process_headers(void const* const file_data, int const file_size, pe_headers* const headers_out);
bool pe_process_import_tables(void const* const file_data, int const file_size, pe_import_tables* const tables_out);
bool pe_process_import_names(void const* const file_data, int const file_size, pe_import_names* const names_in_out);
bool pe_process_import_iat(void const* const file_data, int const file_size, pe_import_iat* const iat_in_out);

bool pe_process_all(void const* const file_data, int const file_size, memory_manager& mm, pe_import_table_info* const iti_in_out);

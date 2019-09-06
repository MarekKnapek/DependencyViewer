#include "pe_test.h"

#include "mz.h"
#include "coff_full.h"
#include "import_table.h"
#include "../assert.h"

#include <cstring>
#include <algorithm>
#include <iterator>

#include <windows.h>


bool pe_test(void const* const& fd, int const& file_size)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const* dos_hdr;
	pe_coff_full_32_64 const* coff_hdr;
	pe_e_parse_mz_header const dos_parsed = pe_parse_mz_header(file_data, file_size, dos_hdr);
	WARN_M_R(dos_parsed == pe_e_parse_mz_header::ok, L"Failed to pe_parse_mz_header.", false);
	bool const coff_parsed = pe_parse_coff_full_32_64(file_data, file_size, coff_hdr);
	WARN_M_R(coff_parsed, L"Failed to pe_parse_coff_full_32_64.", false);
	bool const is_32 = coff_hdr->m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;

	pe_import_directory_table idt;
	bool const import_descriptor_parsed = pe_parse_import_table(file_data, file_size, idt);
	WARN_M_R(import_descriptor_parsed, L"Failed to parse import descriptor", false);
	for(int i = 0; i != idt.m_count; ++i)
	{
		pe_string dldll;
		bool const dll_parsed = pe_parse_import_dll_name(file_data, file_size, idt.m_table[i], dldll);
		WARN_M_R(dll_parsed, L"Failed to parse import DLL name.", false);
		pe_import_address_table iat;
		bool const iat_parsed = pe_parse_import_address_table(file_data, file_size, idt.m_table[i], iat);
		WARN_M_R(iat_parsed, L"Failed to parse import address table.", false);
		for(int j = 0; j != iat.m_count; ++j)
		{
			bool is_ordinal;
			std::uint16_t ordinal;
			pe_hint_name hint_name;
			bool const ia_parsed = pe_parse_import_address(file_data, file_size, iat, j, is_ordinal, ordinal, hint_name);
			WARN_M_R(ia_parsed, L"Failed to parse address import.", false);
		}
	}

	pe_delay_import_table dlit;
	bool const delay_descriptor_parsed = pe_parse_delay_import_table(file_data, file_size, dlit);
	WARN_M_R(delay_descriptor_parsed, L"Failed to parse delay import descriptor", false);
	for(int i = 0; i != dlit.m_count; ++i)
	{
		pe_string dldll;
		bool const dldll_parsed = pe_parse_delay_import_dll_name(file_data, file_size, dlit.m_table[i], dldll);
		WARN_M_R(dldll_parsed, L"Failed to parse delay load DLL name.", false);
		pe_delay_load_import_address_table dliat;
		bool const dliat_parsed = pe_parse_delay_import_address_table(file_data, file_size, dlit.m_table[i], dliat);
		WARN_M_R(dliat_parsed, L"Failed to parse delay load import address table.", false);
		for(int j = 0; j != dliat.m_count; ++j)
		{
			bool is_ordinal;
			std::uint16_t ordinal;
			pe_hint_name hint_name;
			bool const dlia_parsed = pe_parse_delay_import_address(file_data, file_size, dlit.m_table[i], dliat, j, is_ordinal, ordinal, hint_name);
			WARN_M_R(dlia_parsed, L"Failed to parse delay load address import.", false);
		}
	}

	return true;
}

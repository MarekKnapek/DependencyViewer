#include "pe_test.h"

#include "mz.h"
#include "coff_full.h"
#include "import_table.h"
#include "../assert.h"


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

	import_directory_table idt;
	bool const import_table_parsed = pe_parse_import_directory_table(file_data, file_size, idt);
	WARN_M_R(import_table_parsed, L"Failed to parse import table.", false);
	if(idt.m_table != nullptr && idt.m_size != 0 && idt.m_sct != nullptr)
	{
		if(is_32)
		{
			for(int i = 0; i != idt.m_size; ++i)
			{
				pe_string dll_name;
				bool const import_dll_name = pe_parse_import_dll_name(file_data, file_size, idt, i, dll_name);
				WARN_M_R(import_table_parsed, L"Import DLL name not found.", false);
				pe_import_lookup_table_32 ilt;
				bool const import_iat = pe_parse_import_lookup_table_32(file_data, file_size, idt, i, ilt);
				WARN_M_R(import_iat, L"Failed to parse import address table.", false);
				for(int j = 0; j != ilt.m_size; ++j)
				{
					bool is_ordinal;
					bool const parsed_entry = pe_parse_import_lookup_entry_32(file_data, file_size, ilt, j, is_ordinal);
					WARN_M_R(parsed_entry, L"Failed to parse is ordinal.", false);
					if(is_ordinal)
					{
						std::uint16_t ordinal;
						bool const parsed_ordinal = pe_parse_import_lookup_entry_ordinal_32(file_data, file_size, ilt, j, ordinal);
						WARN_M_R(parsed_entry, L"Failed to parse ordinal.", false);
					}
					else
					{
						pe_hint_name hntnm;
						bool const parsed_hint_name = pe_parse_import_lookup_entry_hint_name_32(file_data, file_size, ilt, j, hntnm);
						WARN_M_R(parsed_hint_name, L"Failed to parse hint name.", false);
					}
				}
			}
		}
		else
		{
			for(int i = 0; i != idt.m_size; ++i)
			{
				pe_string dll_name;
				bool const import_dll_name = pe_parse_import_dll_name(file_data, file_size, idt, i, dll_name);
				WARN_M_R(import_table_parsed, L"Import DLL name not found.", false);
				pe_import_lookup_table_64 ilt;
				bool const import_iat = pe_parse_import_lookup_table_64(file_data, file_size, idt, i, ilt);
				WARN_M_R(import_iat, L"Failed to parse import address table.", false);
				for(int j = 0; j != ilt.m_size; ++j)
				{
					bool is_ordinal;
					bool const parsed_entry = pe_parse_import_lookup_entry_64(file_data, file_size, ilt, j, is_ordinal);
					WARN_M_R(parsed_entry, L"Failed to parse is ordinal.", false);
					if(is_ordinal)
					{
						std::uint16_t ordinal;
						bool const parsed_ordinal = pe_parse_import_lookup_entry_ordinal_64(file_data, file_size, ilt, j, ordinal);
						WARN_M_R(parsed_entry, L"Failed to parse ordinal.", false);
					}
					else
					{
						pe_hint_name hntnm;
						bool const parsed_hint_name = pe_parse_import_lookup_entry_hint_name_64(file_data, file_size, ilt, j, hntnm);
						WARN_M_R(parsed_hint_name, L"Failed to parse hint name.", false);
					}
				}
			}
		}
	}

	return true;
}

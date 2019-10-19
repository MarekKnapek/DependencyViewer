#include "pe2.h"

#include "array_bool.h"
#include "assert.h"


bool pe_process_headers(void const* const fd, int const file_size, pe_headers* const headers_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const* dos_header;
	pe_e_parse_mz_header const dos_parsed = pe_parse_mz_header(file_data, file_size, &dos_header);
	WARN_M_R(dos_parsed == pe_e_parse_mz_header::ok, L"Failed to parse MZ header.", false);
	pe_coff_full_32_64 const* coff_header;
	bool const coff_parsed = pe_parse_coff_full_32_64(file_data, file_size, &coff_header);
	WARN_M_R(coff_parsed, L"Failed to parse COFF header.", false);
	headers_out->m_dos = dos_header;
	headers_out->m_coff = coff_header;
	return true;
}

bool pe_process_import_tables(void const* const fd, int const file_size, pe_import_tables* const tables_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_import_directory_table idt;
	bool const import_table_parsed = pe_parse_import_table(file_data, file_size, &idt);
	WARN_M_R(import_table_parsed, L"Failed to parse import table.", false);
	pe_delay_import_table didt;
	bool const dimport_table_parsed = pe_parse_delay_import_table(file_data, file_size, &didt);
	WARN_M_R(dimport_table_parsed, L"Failed to parse delay import table.", false);
	tables_out->m_idt = idt;
	tables_out->m_didt = didt;
	return true;
}

bool pe_process_import_names(void const* const fd, int const file_size, pe_import_names* const names_in_out)
{
	assert(names_in_out);
	char const* const file_data = static_cast<char const*>(fd);
	std::uint16_t const n1 = names_in_out->m_tables->m_idt.m_count;
	std::uint16_t const n2 = names_in_out->m_tables->m_didt.m_count;
	std::uint16_t const n = n1 + n2;
	string const** const strings = names_in_out->m_alc->allocate_objects<string const*>(n);
	int ii = 0;
	for(int i = 0; i != n1; ++i, ++ii)
	{
		pe_string dll_name;
		bool const name_parsed = pe_parse_import_dll_name(file_data, file_size, names_in_out->m_tables->m_idt.m_table[i], &dll_name);
		WARN_M_R(name_parsed, L"Failed to parse import DLL name.", false);
		strings[ii] = names_in_out->m_ustrings->add_string(dll_name.m_str, dll_name.m_len, *names_in_out->m_alc);
	}
	for(int i = 0; i != n2; ++i, ++ii)
	{
		pe_string dll_name;
		bool const name_parsed = pe_parse_delay_import_dll_name(file_data, file_size, names_in_out->m_tables->m_didt.m_table[i], &dll_name);
		WARN_M_R(name_parsed, L"Failed to parse delay import DLL name.", false);
		strings[ii] = names_in_out->m_ustrings->add_string(dll_name.m_str, dll_name.m_len, *names_in_out->m_alc);
	}
	names_in_out->m_names_out = strings;
	return true;
}

bool pe_process_import_iat(void const* const fd, int const file_size, pe_import_iat* const iat_in_out)
{
	assert(iat_in_out);
	char const* const file_data = static_cast<char const*>(fd);
	int const n_dlls = iat_in_out->m_tables->m_idt.m_count + iat_in_out->m_tables->m_didt.m_count;
	std::uint16_t* import_counts = iat_in_out->m_alc->allocate_objects<std::uint16_t>(n_dlls);
	unsigned** are_ordinals_all = iat_in_out->m_alc->allocate_objects<unsigned*>(n_dlls);
	std::uint16_t** ordinals_or_hints_all = iat_in_out->m_alc->allocate_objects<std::uint16_t*>(n_dlls);
	string const*** names_all = iat_in_out->m_alc->allocate_objects<string const**>(n_dlls);
	std::uint16_t** matched_exports_all = iat_in_out->m_alc->allocate_objects<std::uint16_t*>(n_dlls);
	int ii = 0;
	for(int i = 0; i != iat_in_out->m_tables->m_idt.m_count; ++i, ++ii)
	{
		pe_import_address_table iat;
		bool const iat_parsed = pe_parse_import_address_table(file_data, file_size, iat_in_out->m_tables->m_idt.m_table[i], &iat);
		WARN_M_R(iat_parsed, L"Failed to parse import address table.", false);
		int const bits_to_dwords = array_bool_space_needed(iat.m_count);
		unsigned* are_ordinals = iat_in_out->m_alc->allocate_objects<unsigned>(bits_to_dwords);
		std::fill(are_ordinals, are_ordinals + bits_to_dwords, 0u);
		std::uint16_t* ordinals_or_hints = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		string const** names = iat_in_out->m_alc->allocate_objects<string const*>(iat.m_count);
		std::uint16_t* matched_exports = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		for(int j = 0; j != iat.m_count; ++j)
		{
			bool is_ordinal;
			std::uint16_t ordinal;
			pe_hint_name hint_name;
			bool const address_parsed = pe_parse_import_address(file_data, file_size, iat, j, &is_ordinal, &ordinal, &hint_name);
			WARN_M_R(address_parsed, L"Failed to parse import address.", false);
			if(is_ordinal)
			{
				array_bool_set(are_ordinals, j);
				ordinals_or_hints[j] = ordinal;
			}
			else
			{
				ordinals_or_hints[j] = hint_name.m_hint;
				names[j] = iat_in_out->m_ustrings->add_string(hint_name.m_name.m_str, hint_name.m_name.m_len, *iat_in_out->m_alc);
			}
		}
		import_counts[ii] = iat.m_count;
		are_ordinals_all[ii] = are_ordinals;
		ordinals_or_hints_all[ii] = ordinals_or_hints;
		names_all[ii] = names;
		matched_exports_all[ii] = matched_exports;
	}
	for(int i = 0; i != iat_in_out->m_tables->m_didt.m_count; ++i, ++ii)
	{
		pe_delay_load_import_address_table iat;
		bool const iat_parsed = pe_parse_delay_import_address_table(file_data, file_size, iat_in_out->m_tables->m_didt.m_table[i], &iat);
		WARN_M_R(iat_parsed, L"Failed to parse delay import address table.", false);
		int const bits_to_dwords = array_bool_space_needed(iat.m_count);
		unsigned* are_ordinals = iat_in_out->m_alc->allocate_objects<unsigned>(bits_to_dwords);
		std::fill(are_ordinals, are_ordinals + bits_to_dwords, 0u);
		std::uint16_t* ordinals_or_hints = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		string const** names = iat_in_out->m_alc->allocate_objects<string const*>(iat.m_count);
		std::uint16_t* matched_exports = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		for(int j = 0; j != iat.m_count; ++j)
		{
			bool is_ordinal;
			std::uint16_t ordinal;
			pe_hint_name hint_name;
			bool const address_parsed = pe_parse_delay_import_address(file_data, file_size, iat_in_out->m_tables->m_didt.m_table[i], iat, j, &is_ordinal, &ordinal, &hint_name);
			WARN_M_R(address_parsed, L"Failed to parse delay import address.", false);
			if(is_ordinal)
			{
				array_bool_set(are_ordinals, j);
				ordinals_or_hints[j] = ordinal;
			}
			else
			{
				ordinals_or_hints[j] = hint_name.m_hint;
				names[j] = iat_in_out->m_ustrings->add_string(hint_name.m_name.m_str, hint_name.m_name.m_len, *iat_in_out->m_alc);
			}
		}
		import_counts[ii] = iat.m_count;
		are_ordinals_all[ii] = are_ordinals;
		ordinals_or_hints_all[ii] = ordinals_or_hints;
		names_all[ii] = names;
		matched_exports_all[ii] = matched_exports;
	}
	iat_in_out->m_iti_out->m_import_counts = import_counts;
	iat_in_out->m_iti_out->m_are_ordinals = are_ordinals_all;
	iat_in_out->m_iti_out->m_ordinals_or_hints = ordinals_or_hints_all;
	iat_in_out->m_iti_out->m_names = names_all;
	iat_in_out->m_iti_out->m_matched_exports = matched_exports_all;
	return true;
}


bool pe_process_all(void const* const fd, int const file_size, memory_manager& mm, pe_import_table_info* const iti_in_out)
{
	assert(iti_in_out);
	char const* const file_data = static_cast<char const*>(fd);

	pe_headers headers;
	bool const headers_parsed = pe_process_headers(file_data, file_size, &headers);
	WARN_M_R(headers_parsed, L"Failed to process headers.", false);

	pe_import_tables tables;
	bool const count_parsed = pe_process_import_tables(file_data, file_size, &tables);
	WARN_M_R(count_parsed, L"Failed to pe_process_import_tables.", false);
	pe_import_table_info iti;
	iti.m_dll_count = tables.m_idt.m_count + tables.m_didt.m_count;
	iti.m_non_delay_dll_count = tables.m_idt.m_count;

	pe_import_names names;
	names.m_tables = &tables;
	names.m_ustrings = &mm.m_strs;
	names.m_alc = &mm.m_alc;
	bool const names_processed = pe_process_import_names(file_data, file_size, &names);
	WARN_M_R(names_processed, L"Failed to pe_process_import_names.", false);
	iti.m_dll_names = names.m_names_out;

	pe_import_iat imports;
	imports.m_headers = &headers;
	imports.m_tables = &tables;
	imports.m_ustrings = &mm.m_strs;
	imports.m_alc = &mm.m_alc;
	imports.m_iti_out = &iti;
	bool const imports_processed = pe_process_import_iat(file_data, file_size, &imports);
	WARN_M_R(imports_processed, L"Failed to pe_process_import_iat.", false);

	*iti_in_out = iti;
	return true;
}

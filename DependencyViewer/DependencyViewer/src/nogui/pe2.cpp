#include "pe2.h"

#include "array_bool.h"
#include "assert_my.h"

#include "pe/resource_table.h"

#include <algorithm>


static constexpr std::uint16_t const s_image_file_dll_ = 0x2000;


bool pe_process_headers(std::byte const* const file_data, int const file_size, pe_headers* const headers_out)
{
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


bool pe_process_import_tables(std::byte const* const file_data, pe_import_tables* const tables_out)
{
	pe_import_directory_table idt;
	bool const import_table_parsed = pe_parse_import_table(file_data, &idt);
	WARN_M_R(import_table_parsed, L"Failed to parse import table.", false);
	pe_delay_import_table didt;
	bool const dimport_table_parsed = pe_parse_delay_import_table(file_data, &didt);
	WARN_M_R(dimport_table_parsed, L"Failed to parse delay import table.", false);
	tables_out->m_idt = idt;
	tables_out->m_didt = didt;
	return true;
}

bool pe_process_import_names(std::byte const* const file_data, pe_import_names* const names_in_out)
{
	assert(names_in_out);
	std::uint16_t const n1 = names_in_out->m_tables->m_idt.m_count;
	std::uint16_t const n2 = names_in_out->m_tables->m_didt.m_count;
	std::uint16_t const n = n1 + n2;
	string_handle* const strings = names_in_out->m_alc->allocate_objects<string_handle>(n);
	int ii = 0;
	for(int i = 0; i != n1; ++i, ++ii)
	{
		pe_string dll_name;
		bool const name_parsed = pe_parse_import_dll_name(file_data, names_in_out->m_tables->m_idt.m_table[i], &dll_name);
		WARN_M_R(name_parsed, L"Failed to parse import DLL name.", false);
		strings[ii] = names_in_out->m_ustrings->add_string(dll_name.m_str, dll_name.m_len, *names_in_out->m_alc);
	}
	for(int i = 0; i != n2; ++i, ++ii)
	{
		pe_string dll_name;
		bool const name_parsed = pe_parse_delay_import_dll_name(file_data, names_in_out->m_tables->m_didt.m_table[i], &dll_name);
		WARN_M_R(name_parsed, L"Failed to parse delay import DLL name.", false);
		strings[ii] = names_in_out->m_ustrings->add_string(dll_name.m_str, dll_name.m_len, *names_in_out->m_alc);
	}
	names_in_out->m_names_out = strings;
	return true;
}

bool pe_process_import_iat(std::byte const* const file_data, pe_import_iat* const iat_in_out)
{
	assert(iat_in_out);
	int const n_dlls = iat_in_out->m_tables->m_idt.m_count + iat_in_out->m_tables->m_didt.m_count;
	std::uint16_t* const import_counts = iat_in_out->m_alc->allocate_objects<std::uint16_t>(n_dlls);
	array_bool* const are_ordinals_all = iat_in_out->m_alc->allocate_objects<array_bool>(n_dlls);
	std::uint16_t** const ordinals_or_hints_all = iat_in_out->m_alc->allocate_objects<std::uint16_t*>(n_dlls);
	string_handle** const names_all = iat_in_out->m_alc->allocate_objects<string_handle*>(n_dlls);
	string_handle** const undecorated_names_all = iat_in_out->m_alc->allocate_objects<string_handle*>(n_dlls);
	std::uint16_t** const matched_exports_all = iat_in_out->m_alc->allocate_objects<std::uint16_t*>(n_dlls);
	int ii = 0;
	for(int i = 0; i != iat_in_out->m_tables->m_idt.m_count; ++i, ++ii)
	{
		pe_import_address_table iat;
		bool const iat_parsed = pe_parse_import_address_table(file_data, iat_in_out->m_tables->m_idt.m_table[i], &iat);
		WARN_M_R(iat_parsed, L"Failed to parse import address table.", false);
		int const bits_to_dwords = array_bool_space_needed(iat.m_count);
		array_bool const are_ordinals{iat_in_out->m_alc->allocate_objects<unsigned>(bits_to_dwords)};
		std::fill(are_ordinals.m_data, are_ordinals.m_data + bits_to_dwords, 0u);
		std::uint16_t* const ordinals_or_hints = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		string_handle* const names = iat_in_out->m_alc->allocate_objects<string_handle>(iat.m_count);
		string_handle* const undecorated_names = iat_in_out->m_alc->allocate_objects<string_handle>(iat.m_count);
		std::uint16_t* const matched_exports = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		assert((std::fill(matched_exports,  matched_exports + iat.m_count, std::uint16_t{0xFFFE}), true));
		for(int j = 0; j != iat.m_count; ++j)
		{
			bool is_ordinal;
			std::uint16_t ordinal;
			pe_hint_name hint_name;
			bool const address_parsed = pe_parse_import_address(file_data, iat, j, &is_ordinal, &ordinal, &hint_name);
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
		undecorated_names_all[ii] = undecorated_names;
		matched_exports_all[ii] = matched_exports;
	}
	for(int i = 0; i != iat_in_out->m_tables->m_didt.m_count; ++i, ++ii)
	{
		pe_delay_load_import_address_table iat;
		bool const iat_parsed = pe_parse_delay_import_address_table(file_data, iat_in_out->m_tables->m_didt.m_table[i], &iat);
		WARN_M_R(iat_parsed, L"Failed to parse delay import address table.", false);
		int const bits_to_dwords = array_bool_space_needed(iat.m_count);
		array_bool const are_ordinals{iat_in_out->m_alc->allocate_objects<unsigned>(bits_to_dwords)};
		std::fill(are_ordinals.m_data, are_ordinals.m_data + bits_to_dwords, 0u);
		std::uint16_t* const ordinals_or_hints = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		string_handle* const names = iat_in_out->m_alc->allocate_objects<string_handle>(iat.m_count);
		string_handle* const undecorated_names = iat_in_out->m_alc->allocate_objects<string_handle>(iat.m_count);
		std::uint16_t* const matched_exports = iat_in_out->m_alc->allocate_objects<std::uint16_t>(iat.m_count);
		assert((std::fill(matched_exports,  matched_exports + iat.m_count, std::uint16_t{0xFFFE}), true));
		for(int j = 0; j != iat.m_count; ++j)
		{
			bool is_ordinal;
			std::uint16_t ordinal;
			pe_hint_name hint_name;
			bool const address_parsed = pe_parse_delay_import_address(file_data, iat_in_out->m_tables->m_didt.m_table[i], iat, j, &is_ordinal, &ordinal, &hint_name);
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
		undecorated_names_all[ii] = undecorated_names;
		matched_exports_all[ii] = matched_exports;
	}
	iat_in_out->m_iti_out->m_import_counts = import_counts;
	iat_in_out->m_iti_out->m_are_ordinals = are_ordinals_all;
	iat_in_out->m_iti_out->m_ordinals_or_hints = ordinals_or_hints_all;
	iat_in_out->m_iti_out->m_names = names_all;
	iat_in_out->m_iti_out->m_undecorated_names = undecorated_names_all;
	iat_in_out->m_iti_out->m_matched_exports = matched_exports_all;
	return true;
}


#pragma warning(push)
#pragma warning(disable:4701)
#pragma warning(disable:4703)
// potentially uninitialized local variable 'name' used
// potentially uninitialized local pointer variable 'name' used
// potentially uninitialized local variable 'frwrdr' used
// potentially uninitialized local pointer variable 'frwrdr' used
bool pe_process_export_eat(std::byte const* const file_data, pe_export_eat* const eat_in_out)
{
	assert(eat_in_out);
	assert(eat_in_out->m_headers);
	assert(eat_in_out->m_ustrings);
	assert(eat_in_out->m_alc);
	assert(eat_in_out->m_tmp_alc);
	assert(eat_in_out->m_eti_out);
	assert(eat_in_out->m_enpt_count_out);
	assert(eat_in_out->m_enpt_out);

	pe_export_directory_table edt;
	bool const edt_parsed = pe_parse_export_directory_table(file_data, &edt);
	WARN_M_R(edt_parsed, L"Failed to parse export directory table.", false);
	if(!edt.m_table || edt.m_table->m_export_address_count == 0)
	{
		*eat_in_out->m_enpt_count_out = 0;
		eat_in_out->m_eti_out->m_count = 0;
		return true;
	}

	bool const is_32 = pe_is_32_bit(eat_in_out->m_headers->m_coff->m_32.m_standard);
	std::uint32_t const coff_size = is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64);
	std::uint32_t const data_dir_offset = eat_in_out->m_headers->m_dos->m_pe_offset + coff_size;
	pe_data_directory const* const dta_dir_table = reinterpret_cast<pe_data_directory const*>(file_data + data_dir_offset);
	std::uint32_t const export_directory_va = dta_dir_table[static_cast<int>(pe_e_directory_table::export_table)].m_va;
	std::uint32_t const export_directory_size = dta_dir_table[static_cast<int>(pe_e_directory_table::export_table)].m_size;

	pe_export_name_pointer_table enpt;
	bool const enpt_parsed = pe_parse_export_name_pointer_table(file_data, edt, &enpt);
	WARN_M_R(enpt_parsed, L"Failed to parse export name pointer table.", false);
	pe_export_ordinal_table eot;
	bool const eot_parsed = pe_parse_export_ordinal_table(file_data, edt, &eot);
	WARN_M_R(eot_parsed, L"Failed to parse export ordinal table.", false);
	WARN_M_R(enpt.m_count == eot.m_count, L"Export name pointer table and export ordinal table are in fact two columns of the same table.", false);
	WARN_M_R(enpt.m_count == 0 || (enpt.m_table && eot.m_table), L"Export name pointer table and export ordinal table are in fact two columns of the same table.", false);
	pe_export_address_table eat;
	bool const eat_parsed = pe_parse_export_address_table(file_data, edt, &eat);
	WARN_M_R(eat_parsed, L"Failed to parse export address table.", false);

	std::uint16_t const eat_count_proper = static_cast<std::uint16_t>(std::count_if(eat.m_table, eat.m_table + eat.m_count, [](pe_export_address_entry const& eae){ return eae.m_export_rva != 0; }));

	std::uint16_t* ordinals = eat_in_out->m_alc->allocate_objects<std::uint16_t>(eat_count_proper);
	int const bits_to_dwords = array_bool_space_needed(eat_count_proper);
	array_bool const are_rvas{eat_in_out->m_alc->allocate_objects<unsigned>(bits_to_dwords)};
	std::fill(are_rvas.m_data, are_rvas.m_data + bits_to_dwords, 0u);
	pe_rva_or_forwarder* const rvas_or_forwarders = eat_in_out->m_alc->allocate_objects<pe_rva_or_forwarder>(eat_count_proper);
	std::uint16_t* const hints = eat_in_out->m_alc->allocate_objects<std::uint16_t>(eat_count_proper);
	string_handle* const names = eat_in_out->m_alc->allocate_objects<string_handle>(eat_count_proper);
	string_handle* const undecorated_names = eat_in_out->m_alc->allocate_objects<string_handle>(eat_count_proper);
	array_bool const are_used{eat_in_out->m_alc->allocate_objects<unsigned>(bits_to_dwords)};
	std::fill(are_used.m_data, are_used.m_data + bits_to_dwords, 0u);

	std::uint16_t* const enpt_ = eat_in_out->m_tmp_alc->allocate_objects<std::uint16_t>(enpt.m_count);
	std::fill(enpt_, enpt_ + enpt.m_count, static_cast<std::uint16_t>(0xFFFF));

	std::uint16_t const ordinal_base = static_cast<std::uint16_t>(edt.m_table->m_ordinal_base);
	std::uint16_t const n = eat.m_count;
	std::uint16_t hints_processed = 0;
	std::uint16_t j = 0;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		std::uint32_t const export_rva = eat.m_table[i].m_export_rva;
		if(export_rva == 0)
		{
			continue;
		}
		std::uint16_t const ordinal = ordinal_base + i;
		std::uint16_t hint;
		pe_string ean;
		string_handle name;
		bool const ean_parsed = pe_parse_export_address_name(file_data, enpt, eot, i, &hint, &ean);
		WARN_M_R(ean_parsed, L"Failed to parse export address name.", false);
		bool const has_name = ean.m_len != 0;
		if(has_name)
		{
			name = eat_in_out->m_ustrings->add_string(ean.m_str, ean.m_len, *eat_in_out->m_alc);
		}
		pe_string forwarder;
		string_handle frwrdr;
		bool const is_fwd = export_rva >= export_directory_va && export_rva < export_directory_va + export_directory_size;
		bool const is_rva = !is_fwd;
		if(is_fwd)
		{
			const bool fwd_parsed = pe_parse_string_rva(file_data, export_rva, &forwarder);
			WARN_M_R(fwd_parsed, L"Failed to parse export forwarder.", false);
			WARN_M_R(forwarder.m_len >= 3, L"Export forwarder is too short.", false);
			WARN_M_R(std::find(forwarder.m_str, forwarder.m_str + forwarder.m_len, '.') != forwarder.m_str + forwarder.m_len, L"Bad export forwarder name format.", false);
			frwrdr = eat_in_out->m_ustrings->add_string(forwarder.m_str, forwarder.m_len, *eat_in_out->m_alc);
		}
		ordinals[j] = ordinal;
		if(is_rva){ array_bool_set(are_rvas, j); };
		if(is_rva){ rvas_or_forwarders[j].m_rva = export_rva; }else{ rvas_or_forwarders[j].m_forwarder = frwrdr; }
		if(has_name){ hints[j] = hint; }else{ hints[j] = 0xFFFF; }
		if(has_name){ names[j] = name; }else{ names[j] = string_handle{nullptr}; }
		if(has_name){ WARN_M_R(enpt_[hint] == 0xFFFF, L"Bad hint.", false); enpt_[hint] = j; ++hints_processed; }else{}
		++j;
	}

	WARN_M_R(hints_processed == enpt.m_count, L"Not all names processed.", false);
	bool const is_sorted = std::is_sorted(enpt_, enpt_ + enpt.m_count, [&](auto const& a, auto const& b)
	{
		assert(a != 0xFFFF);
		assert(b != 0xFFFF);
		assert(names[a]);
		assert(names[b]);
		return names[a] < names[b];
	});
	WARN_M_R(is_sorted, L"Export name pointer table is not sorted.", false);

	eat_in_out->m_eti_out->m_count = eat_count_proper;
	eat_in_out->m_eti_out->m_ordinal_base = ordinal_base;
	eat_in_out->m_eti_out->m_ordinals = ordinals;
	eat_in_out->m_eti_out->m_are_rvas = are_rvas;
	eat_in_out->m_eti_out->m_rvas_or_forwarders = rvas_or_forwarders;
	eat_in_out->m_eti_out->m_hints = hints;
	eat_in_out->m_eti_out->m_names = names;
	eat_in_out->m_eti_out->m_undecorated_names = undecorated_names;
	eat_in_out->m_eti_out->m_are_used = are_used;
	*eat_in_out->m_enpt_count_out = enpt.m_count;
	*eat_in_out->m_enpt_out = enpt_;
	return true;
}
#pragma warning(pop)


bool pe_process_resource_manifest(std::byte const* const file_data, bool const is_dll, std::uint32_t* const manifest_id_out)
{
	assert(manifest_id_out);
	pe_resource_directory_table const* res_dir_tbl;
	pe_section_header const* res_sct;
	bool const res_dir_tbl_parsed = pe_parse_resource_root_directory_table(file_data, &res_dir_tbl, &res_sct);
	WARN_M_R(res_dir_tbl_parsed, L"Failed to pe_parse_resource_root_directory_table.", false);
	if(res_dir_tbl)
	{
		for(std::uint16_t i = 0; i != res_dir_tbl->m_number_of_id_entries; ++i)
		{
			std::uint32_t type_id;
			bool const type_id_parsed = pe_parse_resource_directory_id_entry(res_dir_tbl, i, &type_id);
			WARN_M_R(type_id_parsed, L"Failed to pe_parse_resource_directory_id_entry.", false);
			if(type_id == 24 /* RT_MANIFEST */)
			{
				pe_resource_directory_table const* manifest_dir_table;
				bool const manifest_dir_table_parsed = pe_parse_resource_sub_directory_table(file_data, *res_sct, res_dir_tbl, res_dir_tbl->m_number_of_name_entries + i, &manifest_dir_table);
				WARN_M_R(manifest_dir_table_parsed, L"Failed to pe_parse_resource_sub_directory_table.", false);
				if(manifest_dir_table->m_number_of_id_entries >= 1)
				{
					// At least one manifest named by integer. We take the first one (they are sorted).
					std::uint32_t name_id;
					bool const name_id_parsed = pe_parse_resource_directory_id_entry(manifest_dir_table, 0, &name_id);
					WARN_M_R(name_id_parsed, L"Failed to pe_parse_resource_directory_id_entry.", false);
					if(name_id >= 1 && name_id <= 16)
					{
						// ID must be between 1 (MINIMUM_RESERVED_MANIFEST_RESOURCE_ID) and 16 (MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID), both inclusive.
						// We don't care about language or data.
						// Skip manifest ID 1 for DLLs.
						if(is_dll && name_id == 1)
						{
							if(manifest_dir_table->m_number_of_id_entries >= 2)
							{
								bool const name_id_parsed_2 = pe_parse_resource_directory_id_entry(manifest_dir_table, 1, &name_id);
								WARN_M_R(name_id_parsed_2, L"Failed to pe_parse_resource_directory_id_entry.", false);
								if(name_id >= 2 && name_id <= 16)
								{
									*manifest_id_out = name_id;
									return true;
								}
							}
						}
						else
						{
							*manifest_id_out = name_id;
							return true;
						}
					}
				}
				break;
			}
		}
	}
	*manifest_id_out = 0;
	return true;
}


bool pe_process_all(std::byte const* const file_data, int const file_size, memory_manager& mm, pe_tables* const tables_in_out)
{
	assert(tables_in_out);
	assert(tables_in_out->m_tmp_alc);
	assert(tables_in_out->m_iti_out);
	assert(tables_in_out->m_eti_out);
	assert(tables_in_out->m_enpt_count_out);
	assert(tables_in_out->m_enpt_out);

	pe_headers headers;
	bool const headers_parsed = pe_process_headers(file_data, file_size, &headers);
	WARN_M_R(headers_parsed, L"Failed to process headers.", false);
	tables_in_out->m_is_32_bit = pe_is_32_bit(headers.m_coff->m_32.m_standard);
	bool is_dll;
	if(tables_in_out->m_is_32_bit)
	{
		is_dll = (headers.m_coff->m_32.m_coff.m_characteristics & s_image_file_dll_) != 0;
	}
	else
	{
		is_dll = (headers.m_coff->m_64.m_coff.m_characteristics & s_image_file_dll_) != 0;
	}

	pe_import_tables tables;
	bool const count_parsed = pe_process_import_tables(file_data, &tables);
	WARN_M_R(count_parsed, L"Failed to pe_process_import_tables.", false);
	pe_import_table_info iti;
	iti.m_normal_dll_count = tables.m_idt.m_count;
	iti.m_delay_dll_count = tables.m_didt.m_count;

	pe_import_names names;
	names.m_tables = &tables;
	names.m_ustrings = &mm.m_strs;
	names.m_alc = &mm.m_alc;
	bool const names_processed = pe_process_import_names(file_data, &names);
	WARN_M_R(names_processed, L"Failed to pe_process_import_names.", false);
	iti.m_dll_names = names.m_names_out;

	pe_import_iat imports;
	imports.m_headers = &headers;
	imports.m_tables = &tables;
	imports.m_ustrings = &mm.m_strs;
	imports.m_alc = &mm.m_alc;
	imports.m_iti_out = &iti;
	bool const imports_processed = pe_process_import_iat(file_data, &imports);
	WARN_M_R(imports_processed, L"Failed to pe_process_import_iat.", false);

	pe_export_table_info eti;
	std::uint16_t entp_count;
	std::uint16_t const* entp;
	pe_export_eat exports;
	exports.m_headers = &headers;
	exports.m_ustrings = &mm.m_strs;
	exports.m_alc = &mm.m_alc;
	exports.m_tmp_alc = tables_in_out->m_tmp_alc;
	exports.m_eti_out = &eti;
	exports.m_enpt_count_out = &entp_count;
	exports.m_enpt_out = &entp;
	bool const export_eat_processed = pe_process_export_eat(file_data, &exports);
	WARN_M_R(export_eat_processed, L"Failed to pe_process_export_eat.", false);

	std::uint32_t manifest_id;
	bool const reources_processed = pe_process_resource_manifest(file_data, is_dll, &manifest_id);
	WARN_M_R(reources_processed, L"Failed to pe_process_resource_manifest.", false);

	*tables_in_out->m_iti_out = iti;
	*tables_in_out->m_eti_out = eti;
	*tables_in_out->m_enpt_count_out = entp_count;
	*tables_in_out->m_enpt_out = entp;
	tables_in_out->m_manifest_id = manifest_id;
	return true;
}

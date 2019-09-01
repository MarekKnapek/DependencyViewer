#include "import_table.h"

#include "mz.h"
#include "coff_full.h"
#include "../assert.h"


template<typename T>
auto pe_min(T const& a, T const& b)
{
	return b < a ? b : a;
}


bool pe_parse_import_directory_table(void const* const& fd, int const& file_size, pe_import_directory_table& idt)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const dir_tbl_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	WARN_M_R(static_cast<int>(pe_e_directory_table::import_table) < dir_tbl_cnt, L"Import table is not present.", false);
	pe_data_directory const* const dir_tbl = reinterpret_cast<pe_data_directory const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	pe_data_directory const& imp_tbl = dir_tbl[static_cast<int>(pe_e_directory_table::import_table)];
	if(imp_tbl.m_va == 0 || imp_tbl.m_size == 0)
	{
		idt.m_table = nullptr;
		idt.m_size = 0;
		idt.m_sct = nullptr;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const imp_dir_tbl_raw = pe_find_object_in_raw(file_data, file_size, imp_tbl.m_va, imp_tbl.m_size, sct);
	WARN_M_R(imp_dir_tbl_raw != 0, L"Import directory table not found in any section.", false);
	std::uint32_t const imp_dir_tbl_cnt_max = imp_tbl.m_size / sizeof(pe_import_directory_entry);
	std::uint32_t imp_dir_tbl_cnt = 0xffffffff;
	pe_import_directory_entry const* const imp_dir_tbl = reinterpret_cast<pe_import_directory_entry const*>(file_data + imp_dir_tbl_raw);
	for(std::uint32_t i = 0; i != imp_dir_tbl_cnt_max; ++i)
	{
		pe_import_directory_entry const& imp_dir =imp_dir_tbl[i];
		if
		(
			imp_dir.m_import_lookup_table == 0 &&
			imp_dir.m_date_time == 0 &&
			imp_dir.m_forwarder_chain == 0 &&
			imp_dir.m_name == 0 &&
			imp_dir.m_import_adress_table == 0
		)
		{
			imp_dir_tbl_cnt = i;
			break;
		}
	}
	WARN_M_R(imp_dir_tbl_cnt != 0xffffffff, L"Could not found import directory table size.", false);
	idt.m_table = imp_dir_tbl;
	idt.m_size = static_cast<int>(imp_dir_tbl_cnt);
	idt.m_sct = sct;
	return true;
}

bool pe_parse_import_dll_name(void const* const& fd, int const& file_size, pe_import_directory_table const& idt, int const& idx, pe_string& str)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_import_directory_entry const& imp_dir = idt.m_table[idx];
	WARN_M_R(imp_dir.m_name != 0, L"Import directory has no DLL name.", false);
	pe_section_header const* sct;
	std::uint32_t const dll_name_raw = pe_find_object_in_raw(file_data, file_size, imp_dir.m_name, 2, sct);
	WARN_M_R(dll_name_raw != 0, L"Import directory DLL name not found in any section.", false);
	char const* const dll_name = file_data + dll_name_raw;
	std::uint32_t const dll_name_len_max = pe_min(256u, sct->m_raw_ptr + sct->m_raw_size - dll_name_raw);
	std::uint32_t dll_name_len = 0xffffffff;
	for(std::uint32_t i = 0; i != dll_name_len_max; ++i)
	{
		if(file_data[dll_name_raw + i] == '\0')
		{
			dll_name_len = i;
			break;
		}
	}
	WARN_M_R(dll_name_len != 0 && dll_name_len != 0xffffffff, L"Could not find import directory DLL name length.", false);
	WARN_M_R(pe_is_ascii(dll_name, dll_name_len), L"DLL name shall be ASCII.", false);
	str.m_str = dll_name;
	str.m_len = dll_name_len;
	return true;
}

bool pe_parse_import_lookup_table_32(void const* const& fd, int const& file_size, pe_import_directory_table const& idt, int const& idx, pe_import_lookup_table_32& ilt)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_import_directory_entry const& imp_dir = idt.m_table[idx];
	std::uint32_t const iat_rva = imp_dir.m_import_lookup_table != 0 ? imp_dir.m_import_lookup_table : imp_dir.m_import_adress_table;
	WARN_M_R(iat_rva != 0, L"Import directory has no import lookup table or import address table.", false);
	WARN_M_R(iat_rva >= idt.m_sct->m_virtual_address && iat_rva < idt.m_sct->m_virtual_address + idt.m_sct->m_virtual_size, L"Import address table shall be in the same section as import directory table.", false); // Not sure about this.
	std::uint32_t const iat_raw = idt.m_sct->m_raw_ptr + (iat_rva - idt.m_sct->m_virtual_address);
	pe_import_lookup_entry_32 const* const iat = reinterpret_cast<pe_import_lookup_entry_32 const*>(file_data + iat_raw);
	std::uint32_t const iat_cnt_max = pe_min(64u * 1024u, idt.m_sct->m_raw_ptr + idt.m_sct->m_raw_size - iat_raw);
	std::uint32_t iat_cnt = 0xffffffff;
	for(std::uint32_t i = 0; i != iat_cnt_max; ++i)
	{
		if(iat[i].m_value == 0)
		{
			iat_cnt = i;
			break;
		}
	}
	WARN_M_R(iat_cnt != 0xffffffff, L"Could not find import address table size.", false);
	ilt.m_table = iat;
	ilt.m_size = static_cast<int>(iat_cnt);
	ilt.m_sct = idt.m_sct;
	return true;
}

bool pe_parse_import_lookup_table_64(void const* const& fd, int const& file_size, pe_import_directory_table const& idt, int const& idx, pe_import_lookup_table_64& ilt)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_import_directory_entry const& imp_dir = idt.m_table[idx];
	std::uint32_t const iat_rva = imp_dir.m_import_lookup_table != 0 ? imp_dir.m_import_lookup_table : imp_dir.m_import_adress_table;
	WARN_M_R(iat_rva != 0, L"Import directory has no import lookup table or import address table.", false);
	WARN_M_R(iat_rva >= idt.m_sct->m_virtual_address && iat_rva < idt.m_sct->m_virtual_address + idt.m_sct->m_virtual_size, L"Import address table shall be in the same section as import directory table.", false); // Not sure about this.
	std::uint32_t const iat_raw = idt.m_sct->m_raw_ptr + (iat_rva - idt.m_sct->m_virtual_address);
	pe_import_lookup_entry_64 const* const iat = reinterpret_cast<pe_import_lookup_entry_64 const*>(file_data + iat_raw);
	std::uint32_t const iat_cnt_max = pe_min(64u * 1024u, idt.m_sct->m_raw_ptr + idt.m_sct->m_raw_size - iat_raw);
	std::uint32_t iat_cnt = 0xffffffff;
	for(std::uint32_t i = 0; i != iat_cnt_max; ++i)
	{
		if(iat[i].m_value == 0)
		{
			iat_cnt = i;
			break;
		}
	}
	WARN_M_R(iat_cnt != 0xffffffff, L"Could not find import address table size.", false);
	ilt.m_table = iat;
	ilt.m_size = static_cast<int>(iat_cnt);
	ilt.m_sct = idt.m_sct;
	return true;
}

bool pe_parse_import_lookup_entry_32(void const* const& file_data, int const& file_size, pe_import_lookup_table_32 const& ilt, int const& idx, bool& is_ordinal)
{
	pe_import_lookup_entry_32 const& ile = ilt.m_table[idx];
	is_ordinal = (ile.m_value & 0x80000000) != 0;
	return true;
}

bool pe_parse_import_lookup_entry_64(void const* const& file_data, int const& file_size, pe_import_lookup_table_64 const& ilt, int const& idx, bool& is_ordinal)
{
	pe_import_lookup_entry_64 const& ile = ilt.m_table[idx];
	is_ordinal = (ile.m_value & 0x8000000000000000ull) != 0;
	return true;
}

bool pe_parse_import_lookup_entry_ordinal_32(void const* const& file_data, int const& file_size, pe_import_lookup_table_32 const& ilt, int const& idx, std::uint16_t& ordinal)
{
	pe_import_lookup_entry_32 const& ile = ilt.m_table[idx];
	WARN_M_R((ile.m_value & 0x7fff0000) == 0, L"Bits 30-15 must be 0.", false);
	ordinal = ile.m_value & 0x0000000f;
	return true;
}

bool pe_parse_import_lookup_entry_ordinal_64(void const* const& file_data, int const& file_size, pe_import_lookup_table_64 const& ilt, int const& idx, std::uint16_t& ordinal)
{
	pe_import_lookup_entry_64 const& ile = ilt.m_table[idx];
	WARN_M_R((ile.m_value & 0x7fffffffffff0000) == 0, L"Bits 62-15 must be 0.", false);
	ordinal = ile.m_value & 0x000000000000000full;
	return true;
}

bool pe_parse_import_lookup_entry_hint_name_32(void const* const& fd, int const& file_size, pe_import_lookup_table_32 const& ilt, int const& idx, pe_hint_name& hntnm)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_import_lookup_entry_32 const& ile = ilt.m_table[idx];
	std::uint32_t const hint_name_rva = ile.m_value & 0x7fffffff;
	return pe_parse_import_lookup_entry_hint_name_impl(file_data, file_size, *ilt.m_sct, hint_name_rva, hntnm);
}

bool pe_parse_import_lookup_entry_hint_name_64(void const* const& fd, int const& file_size, pe_import_lookup_table_64 const& ilt, int const& idx, pe_hint_name& hntnm)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_import_lookup_entry_64 const& ile = ilt.m_table[idx];
	WARN_M_R((ile.m_value & 0x7FFFFFFF80000000ull) == 0, L"Bits 62-31 must be 0.", false);
	std::uint32_t const hint_name_rva = ile.m_value & 0x000000007fffffffull;
	return pe_parse_import_lookup_entry_hint_name_impl(file_data, file_size, *ilt.m_sct, hint_name_rva, hntnm);
}

bool pe_parse_import_lookup_entry_hint_name_impl(void const* const& fd, int const& file_size, pe_section_header const& sct, std::uint32_t const& hint_name_rva, pe_hint_name& hntnm)
{
	char const* const file_data = static_cast<char const*>(fd);
	WARN_M_R(hint_name_rva >= sct.m_virtual_address && hint_name_rva < sct.m_virtual_address + sct.m_virtual_size, L"Hint/Name shall be in the same section as import lookup table.", false); // Not sure about this.
	std::uint32_t const hint_name_raw = sct.m_raw_ptr + (hint_name_rva - sct.m_virtual_address);
	std::uint16_t const& hint = *reinterpret_cast<std::uint16_t const*>(file_data + hint_name_raw);
	char const* const name = file_data + hint_name_raw + sizeof(std::uint16_t);
	std::uint32_t const name_len_max = pe_min(32u * 1024u, sct.m_raw_ptr + sct.m_raw_size - hint_name_raw - static_cast<int>(sizeof(std::uint16_t)));
	std::uint32_t name_len = 0xffffffff;
	for(std::uint32_t i = 0; i != name_len_max; ++i)
	{
		if(name[i] == '\0')
		{
			name_len = i;
			break;
		}
	}
	WARN_M_R(name_len != 0xffffffff, L"Could not find import name length.", false);
	WARN_M_R(pe_is_ascii(name, static_cast<int>(name_len)), L"Import name shall be ASCII.", false);
	hntnm.m_hint = hint;
	hntnm.m_name.m_str = name;
	hntnm.m_name.m_len = static_cast<int>(name_len);
	return true;
}

bool pe_parse_delay_import_descriptor(void const* const& fd, int const& file_size, pe_delay_import_descriptor& did)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const dir_tbl_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	WARN_M_R(static_cast<int>(pe_e_directory_table::delay_import_descriptor) < dir_tbl_cnt, L"Delay import table is not present.", false);
	pe_data_directory const* const dir_tbl = reinterpret_cast<pe_data_directory const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	pe_data_directory const& dimp_tbl = dir_tbl[static_cast<int>(pe_e_directory_table::delay_import_descriptor)];
	if(dimp_tbl.m_va == 0 || dimp_tbl.m_size == 0)
	{
		did.m_table = nullptr;
		did.m_size = 0;
		did.m_sct = nullptr;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const dimp_dir_tbl_raw = pe_find_object_in_raw(file_data, file_size, dimp_tbl.m_va, dimp_tbl.m_size, sct);
	WARN_M_R(dimp_dir_tbl_raw != 0, L"Delay import directory table not found in any section.", false);
	std::uint32_t const dimp_dir_tbl_cnt_max = dimp_tbl.m_size / sizeof(pe_delay_load_directory_entry);
	std::uint32_t dimp_dir_tbl_cnt = 0xffffffff;
	pe_delay_load_directory_entry const* const dimp_dir_tbl = reinterpret_cast<pe_delay_load_directory_entry const*>(file_data + dimp_dir_tbl_raw);
	for(std::uint32_t i = 0; i != dimp_dir_tbl_cnt_max; ++i)
	{
		pe_delay_load_directory_entry const& dimp_dir = dimp_dir_tbl[i];
		if
		(
			dimp_dir.m_attributes == 0 &&
			dimp_dir.m_name == 0 &&
			dimp_dir.m_module_handle == 0 &&
			dimp_dir.m_delay_import_address_table == 0 &&
			dimp_dir.m_delay_import_name_table == 0 &&
			dimp_dir.m_bound_delay_import_table == 0 &&
			dimp_dir.m_unload_delay_import_table == 0 &&
			dimp_dir.m_timestamp == 0
		)
		{
			dimp_dir_tbl_cnt = i;
			break;
		}
	}
	WARN_M_R(dimp_dir_tbl_cnt != 0xffffffff, L"Could not found delay import directory table size.", false);
	did.m_table = dimp_dir_tbl;
	did.m_size = static_cast<int>(dimp_dir_tbl_cnt);
	did.m_sct = sct;
	return true;
}

bool pe_parse_delay_import_dll_name_32(void const* const& fd, int const& file_size, pe_delay_import_descriptor const& did, int const& idx, pe_string& str)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	pe_delay_load_directory_entry const& dimp = did.m_table[idx];
	WARN_M_R(dimp.m_name != 0, L"Delay import directory has no DLL name.", false);
	std::uint32_t const dll_name_rva = (dimp.m_attributes & 1u) != 0 ? dimp.m_name : dimp.m_name - coff_hdr.m_32.m_windows.m_image_base;
	return pe_parse_delay_import_dll_name_impl(file_data, file_size, dll_name_rva, str);
}

bool pe_parse_delay_import_dll_name_64(void const* const& fd, int const& file_size, pe_delay_import_descriptor const& did, int const& idx, pe_string& str)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	pe_delay_load_directory_entry const& dimp = did.m_table[idx];
	WARN_M_R(dimp.m_name != 0, L"Delay import directory has no DLL name.", false);
	WARN_M_R(coff_hdr.m_64.m_windows.m_image_base < dimp.m_name, L"Image base is too damn high.", false);
	std::uint32_t const dll_name_rva = (dimp.m_attributes & 1u) != 0 ? dimp.m_name : dimp.m_name - static_cast<std::uint32_t>(coff_hdr.m_64.m_windows.m_image_base);
	return pe_parse_delay_import_dll_name_impl(file_data, file_size, dll_name_rva, str);
}

bool pe_parse_delay_import_dll_name_impl(void const* const& fd, int const& file_size, std::uint32_t const& delay_dll_name_rva, pe_string& str)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_section_header const* sct;
	std::uint32_t const dll_name_raw = pe_find_object_in_raw(file_data, file_size, delay_dll_name_rva, 2, sct);
	WARN_M_R(dll_name_raw != 0, L"Delay import directory DLL name not found in any section.", false);
	char const* const dll_name = file_data + dll_name_raw;
	std::uint32_t const dll_name_len_max = pe_min(256u, sct->m_raw_ptr + sct->m_raw_size - dll_name_raw);
	std::uint32_t dll_name_len = 0xffffffff;
	for(std::uint32_t i = 0; i != dll_name_len_max; ++i)
	{
		if(file_data[dll_name_raw + i] == '\0')
		{
			dll_name_len = i;
			break;
		}
	}
	WARN_M_R(dll_name_len != 0 && dll_name_len != 0xffffffff, L"Could not find delay import directory DLL name length.", false);
	WARN_M_R(pe_is_ascii(dll_name, dll_name_len), L"Delay DLL name shall be ASCII.", false);
	str.m_str = dll_name;
	str.m_len = dll_name_len;
	return true;
}

#include "export_table.h"

#include "coff_full.h"
#include "mz.h"
#include "pe_util.h"

#include "../assert.h"

#include <algorithm>


bool operator==(pe_export_ordinal_entry const& a, pe_export_ordinal_entry const& b)
{
	return a.m_idx_to_eat == b.m_idx_to_eat;
}


bool pe_parse_export_directory_table(void const* const fd, int const file_size, pe_export_directory_table& edt_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const dir_tbl_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	if(static_cast<int>(pe_e_directory_table::export_table) >= dir_tbl_cnt)
	{
		edt_out.m_table = nullptr;
		return true;
	}
	pe_data_directory const* const dir_tbl = reinterpret_cast<pe_data_directory const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	pe_data_directory const& exp_tbl = dir_tbl[static_cast<int>(pe_e_directory_table::export_table)];
	if(exp_tbl.m_va == 0 || exp_tbl.m_size == 0)
	{
		edt_out.m_table = nullptr;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const exp_dir_tbl_raw = pe_find_object_in_raw(file_data, file_size, exp_tbl.m_va, exp_tbl.m_size, sct);
	WARN_M_R(exp_dir_tbl_raw != 0, L"Export directory table not found in any section.", false);
	pe_export_directory_entry const* const edt = reinterpret_cast<pe_export_directory_entry const*>(file_data + exp_dir_tbl_raw);
	WARN_M_R(edt->m_ordinal_base <= 0xFFFF, L"Ordinal base is too high.", false);
	WARN_M_R(edt->m_export_address_count <= 0xFFFF, L"Too many addresses to export.", false);
	WARN_M_R(edt->m_ordinal_base + edt->m_export_address_count <= 0xFFFF, L"Biggest ordinal is too high.", false);
	WARN_M_R(edt->m_names_count <= edt->m_export_address_count, L"More names than exported addresses.", false);
	WARN_M_R(edt->m_names_count == 0 || (edt->m_export_name_table_rva != 0 && edt->m_ordinal_table_rva != 0), L"Export name pointer table and export ordinal table are actually two columns of single table.", false);
	WARN_M_R(edt->m_export_address_count == 0 || edt->m_export_address_table_rva != 0, L"If export address table has size it must also have body.", false);
	edt_out.m_table = edt;
	return true;
}

bool pe_parse_export_name_pointer_table(void const* const fd, int const file_size, pe_export_directory_table const& edt, pe_export_name_pointer_table& enpt_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	if(edt.m_table->m_export_name_table_rva == 0)
	{
		enpt_out.m_table = nullptr;
		enpt_out.m_count = 0;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const enpt_raw = pe_find_object_in_raw(file_data, file_size, edt.m_table->m_export_name_table_rva, edt.m_table->m_names_count * sizeof(pe_export_name_pointer_entry), sct);
	WARN_M_R(enpt_raw != 0, L"Export name pointer table not found in any section.", false);
	pe_export_name_pointer_entry const* enpt = reinterpret_cast<pe_export_name_pointer_entry const*>(file_data + enpt_raw);
	enpt_out.m_table = enpt;
	enpt_out.m_count = static_cast<int>(edt.m_table->m_names_count);
	return true;
}

bool pe_parse_export_ordinal_table(void const* const fd, int const file_size, pe_export_directory_table const& edt, pe_export_ordinal_table& eot_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	if(edt.m_table->m_ordinal_table_rva == 0)
	{
		eot_out.m_table = nullptr;
		eot_out.m_count = 0;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const eot_raw = pe_find_object_in_raw(file_data, file_size, edt.m_table->m_ordinal_table_rva, edt.m_table->m_names_count * sizeof(pe_export_ordinal_entry), sct);
	WARN_M_R(eot_raw != 0, L"Export ordinal table not found in any section.", false);
	pe_export_ordinal_entry const* eot = reinterpret_cast<pe_export_ordinal_entry const*>(file_data + eot_raw);
	eot_out.m_table = eot;
	eot_out.m_count = static_cast<int>(edt.m_table->m_names_count);
	return true;
}

bool pe_parse_export_address_table(void const* const fd, int const file_size, pe_export_directory_table const& edt, pe_export_address_table& eat_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	if(edt.m_table->m_export_address_table_rva == 0)
	{
		eat_out.m_table = nullptr;
		eat_out.m_count = 0;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const eot_raw = pe_find_object_in_raw(file_data, file_size, edt.m_table->m_export_address_table_rva, edt.m_table->m_export_address_count * sizeof(pe_export_address_entry), sct);
	WARN_M_R(eot_raw != 0, L"Export address table not found in any section.", false);
	pe_export_address_entry const* eat = reinterpret_cast<pe_export_address_entry const*>(file_data + eot_raw);
	eat_out.m_table = eat;
	eat_out.m_count = static_cast<int>(edt.m_table->m_export_address_count);
	return true;
}

bool pe_parse_export_address_name(void const* const fd, int const file_size, pe_export_name_pointer_table const& enpt, pe_export_ordinal_table const& eot, std::uint16_t const& idx, std::uint16_t& hint_out, pe_string& ean_out)
{
	char const* const file_data = static_cast<char const*>(fd);
	if(enpt.m_count == 0)
	{
		hint_out = 0xffff;
		ean_out.m_str = nullptr;
		ean_out.m_len = 0;
		return true;
	}
	auto const it = std::find(eot.m_table, eot.m_table + eot.m_count, pe_export_ordinal_entry{idx});
	if(it == eot.m_table + eot.m_count)
	{
		hint_out = 0xffff;
		ean_out.m_str = nullptr;
		ean_out.m_len = 0;
		return true;
	}
	std::uint16_t const hint = static_cast<std::uint16_t>(it - eot.m_table);
	std::uint32_t const export_address_name_rva = enpt.m_table[hint].m_export_address_name_rva;
	pe_section_header const* sct;
	std::uint32_t const export_address_name_raw = pe_find_object_in_raw(file_data, file_size, export_address_name_rva, 2, sct);
	WARN_M_R(export_address_name_raw != 0, L"Export address name not found in any section.", false);
	char const* const export_address_name = reinterpret_cast<char const*>(file_data + export_address_name_raw);
	static constexpr const std::uint32_t s_export_address_name_max_len = 32 * 1024;
	std::uint32_t const export_address_name_len_max = std::min<std::uint32_t>(s_export_address_name_max_len, sct->m_raw_ptr + sct->m_raw_size - export_address_name_raw);
	auto const export_address_name_end = std::find(export_address_name, export_address_name + export_address_name_len_max, L'\0');
	WARN_M_R(export_address_name_end != export_address_name + export_address_name_len_max, L"Could not find export address name length.", false);
	std::uint16_t const export_address_name_len = static_cast<std::uint16_t>(export_address_name_end - export_address_name);
	WARN_M_R(export_address_name_len >= 1, L"Export address name is too short.", false);
	WARN_M_R(pe_is_ascii(export_address_name, export_address_name_len), L"Export address name shall be ASCII.", false);
	hint_out = hint;
	ean_out.m_str = export_address_name;
	ean_out.m_len = export_address_name_len;
	return true;
}

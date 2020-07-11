#include "resource_table.h"

#include "coff_full.h"
#include "mz.h"
#include "pe_util.h"

#include "../assert_my.h"
#include "../cassert_my.h"


bool pe_parse_resource_root_directory_table(std::byte const* const file_data, pe_resource_directory_table const** const res_root_dir_tbl_out, pe_section_header const** const res_sct_out)
{
	assert(res_root_dir_tbl_out);
	assert(res_sct_out);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = pe_is_32_bit(coff_hdr.m_32.m_standard);
	std::uint32_t const dir_tbl_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	if(!(static_cast<int>(pe_e_directory_table::resource_table) < dir_tbl_cnt))
	{
		*res_root_dir_tbl_out = nullptr;
		return true;
	}
	pe_data_directory const* const dir_tbl = reinterpret_cast<pe_data_directory const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	pe_data_directory const& res_dir = dir_tbl[static_cast<int>(pe_e_directory_table::resource_table)];
	if(res_dir.m_va == 0 || res_dir.m_size == 0)
	{
		*res_root_dir_tbl_out = nullptr;
		return true;
	}
	pe_section_header const* sct;
	std::uint32_t const res_dir_tbl_raw = pe_find_object_in_raw(file_data, res_dir.m_va, res_dir.m_size, sct);
	if(!res_dir_tbl_raw)
	{
		*res_root_dir_tbl_out = nullptr;
		return true;
	}
	std::uint32_t const res_root_dir_tbl_offset = res_dir_tbl_raw - sct->m_raw_ptr;
	pe_resource_directory_table const* res_root_dir_tbl;
	bool const res_root_dir_tbl_parsed = pe_parse_resource_directory_table(file_data, *sct, res_root_dir_tbl_offset, &res_root_dir_tbl);
	WARN_M_R(res_root_dir_tbl_parsed, L"Failed to pe_parse_resource_directory_table.", false);
	*res_root_dir_tbl_out = res_root_dir_tbl;
	*res_sct_out = sct;
	return true;
}

bool pe_parse_resource_directory_table(std::byte const* const file_data, pe_section_header const& res_sct, std::uint32_t const dir_tbl_offset, pe_resource_directory_table const** res_dir_tbl_out)
{
	assert(file_data);
	assert(res_dir_tbl_out);
	WARN_M_R(dir_tbl_offset < res_sct.m_raw_size, L"Out of bounds.", false);
	WARN_M_R(sizeof(pe_resource_directory_table) <= res_sct.m_raw_size - dir_tbl_offset, L"Not enough room.", false);
	std::uint32_t const res_dir_tbl_raw = res_sct.m_raw_ptr + dir_tbl_offset;
	pe_resource_directory_table const* const res_dir_tbl = reinterpret_cast<pe_resource_directory_table const*>(file_data + res_dir_tbl_raw);
	WARN_M(res_dir_tbl->m_characteristics == 0, L"Resource directory table shall have zero characteristics.");
	std::uint32_t const entries_offset = dir_tbl_offset + sizeof(pe_resource_directory_table);
	std::uint32_t const entries_count = static_cast<std::uint32_t>(res_dir_tbl->m_number_of_name_entries) + static_cast<std::uint32_t>(res_dir_tbl->m_number_of_id_entries);
	WARN_M_R(entries_offset < res_sct.m_raw_size, L"Out of bounds.", false);
	WARN_M_R(entries_count * sizeof(pe_resource_directory_entry) <= res_sct.m_raw_size - entries_offset, L"Not enough room.", false);
	*res_dir_tbl_out = res_dir_tbl;
	return true;
}

bool pe_parse_resource_directory_id_entry(pe_resource_directory_table const* const res_dir_tbl, std::uint16_t const idx, std::uint32_t* const entry_id_out)
{
	assert(res_dir_tbl);
	assert(entry_id_out);
	std::byte const* const entries_begin = reinterpret_cast<std::byte const*>(res_dir_tbl) + sizeof(pe_resource_directory_table);
	pe_resource_directory_entry const* const entries = reinterpret_cast<pe_resource_directory_entry const*>(entries_begin);
	pe_resource_directory_entry const& entry = entries[res_dir_tbl->m_number_of_name_entries + idx];
	WARN_M_R((entry.m_integer_id & (1u << 31)) == 0, L"Resource integer id shall have high bit cleared.", false);
	std::uint32_t const entry_id = entry.m_integer_id;
	*entry_id_out = entry_id;
	return true;
}

bool pe_parse_resource_sub_directory_table(std::byte const* const file_data, pe_section_header const& res_sct, pe_resource_directory_table const* const res_dir_tbl, std::uint16_t const idx, pe_resource_directory_table const** const sub_dir_tbl_out)
{
	assert(res_dir_tbl);
	assert(sub_dir_tbl_out);
	std::byte const* const entries_begin = reinterpret_cast<std::byte const*>(res_dir_tbl) + sizeof(pe_resource_directory_table);
	pe_resource_directory_entry const* const entries = reinterpret_cast<pe_resource_directory_entry const*>(entries_begin);
	pe_resource_directory_entry const& entry = entries[idx];
	WARN_M_R((entry.m_subdirectory_offset & (1u << 31)) != 0, L"Resource sub directory offset shall have high bit set.", false);
	std::uint32_t const sub_dir_tbl_offset = entry.m_subdirectory_offset &~ (1u << 31);
	pe_resource_directory_table const* sub_dir_tbl;
	bool const sub_dir_tbl_parsed = pe_parse_resource_directory_table(file_data, res_sct, sub_dir_tbl_offset, &sub_dir_tbl);
	WARN_M_R(sub_dir_tbl_parsed, L"Failed to pe_parse_resource_directory_table.", false);
	*sub_dir_tbl_out = sub_dir_tbl;
	return true;
}

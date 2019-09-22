#include "pe_util.h"

#include "../assert.h"
#include "mz.h"

#include <algorithm>


std::uint32_t pe_find_object_in_raw(void const* const& fd, int const& /*file_size*/, std::uint32_t const& obj_va, std::uint32_t const& obj_size, pe_section_header const*& sct)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dos_hdr = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_full_32_64 const& coff_hdr = *reinterpret_cast<pe_coff_full_32_64 const*>(file_data + dos_hdr.m_pe_offset);
	bool const is_32 = coff_hdr.m_32.m_standard.m_signature == s_pe_coff_optional_sig_32;
	std::uint32_t const data_dir_cnt = is_32 ? coff_hdr.m_32.m_windows.m_data_directory_count : coff_hdr.m_64.m_windows.m_data_directory_count;
	std::uint32_t const sect_tbl_cnt = is_32 ? coff_hdr.m_32.m_coff.m_section_count : coff_hdr.m_64.m_coff.m_section_count;
	pe_section_header const* const sect_tbl = reinterpret_cast<pe_section_header const*>(file_data + dos_hdr.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)) + data_dir_cnt * sizeof(pe_data_directory));
	for(std::uint32_t i = 0; i != sect_tbl_cnt; ++i)
	{
		pe_section_header const& sect = sect_tbl[i];
		if(obj_va >= sect.m_virtual_address && obj_va < sect.m_virtual_address + sect.m_raw_size)
		{
			std::uint32_t const offset_iniside_sect = obj_va - sect.m_virtual_address;
			std::uint32_t const obj_raw = sect.m_raw_ptr + offset_iniside_sect;
			WARN_M_R(obj_raw + obj_size <= sect.m_raw_ptr + sect.m_raw_size, L"Object does not fin in section raw size.", 0);
			sct = &sect;
			return obj_raw;
		}
	}
	WARN_M_R(false, L"Object not found in any section.", 0);
}

bool pe_is_ascii(char const* const& str, int const& len)
{
	return std::all_of(str, str + len, [](char const& e){ return e >= 32 && e <= 126; });
}

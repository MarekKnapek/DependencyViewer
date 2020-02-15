#include "coff_full.h"

#include "mz.h"

#include "../assert_my.h"


bool pe_parse_coff_full_32_64(std::byte const* const file_data, int const file_size, pe_coff_full_32_64 const** const header_out)
{
	assert(header_out);
	pe_dos_header const& dosheader = *reinterpret_cast<pe_dos_header const*>(file_data);

	pe_coff_header const* coff_hdr;
	pe_e_parse_coff_header const coff = pe_parse_coff_header(file_data, file_size, &coff_hdr);
	WARN_M_R(coff == pe_e_parse_coff_header::ok, L"Failed to pe_parse_coff_header.", false);

	pe_coff_optional_header_standard_32_64 const* coff_opt_std;
	pe_e_parse_coff_optional_header_standard_32_64 const coff_optional = pe_parse_coff_optional_header_standard_32_64(file_data, file_size, &coff_opt_std);
	WARN_M_R(coff_optional == pe_e_parse_coff_optional_header_standard_32_64::ok, L"Failed to pe_parse_coff_optional_header_standard_32_64.", false);

	pe_coff_optional_header_windows_32_64 const* coff_opt_win;
	pe_e_parse_coff_optional_header_windows_32_64 const coff_windows = pe_parse_coff_optional_header_windows_32_64(file_data, file_size, &coff_opt_win);
	WARN_M_R(coff_windows == pe_e_parse_coff_optional_header_windows_32_64::ok, L"Failed to pe_parse_coff_optional_header_windows_32_64.", false);

	bool const is_32 = pe_is_32_bit(coff_opt_std->m_32);
	std::uint32_t const& dir_cnt = is_32 ? coff_opt_win->m_32.m_data_directory_count : coff_opt_win->m_64.m_data_directory_count;
	WARN_M_R(dir_cnt <= 16, L"Too many data directories.", false);
	WARN_M_R(coff_hdr->m_optional_header_size == (is_32 ? (sizeof(pe_coff_optional_header_standard_32) + sizeof(pe_coff_optional_header_windows_32)) : (sizeof(pe_coff_optional_header_standard_64) + sizeof(pe_coff_optional_header_windows_64))) + dir_cnt * sizeof(pe_data_directory), L"Optional header size is too small to contain coff_full_32_64.", false);
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + sizeof(pe_coff_full_32_64)), L"File too small to contain coff_full_32_64.", false);
	pe_coff_full_32_64 const& header = *reinterpret_cast<pe_coff_full_32_64 const*>(coff_hdr);
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64) + dir_cnt * sizeof(pe_data_directory))), L"File too small to contain all directories.", false);
	pe_data_directory const* const directories = reinterpret_cast<pe_data_directory const*>(file_data + dosheader.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)));
	WARN_M(dir_cnt < static_cast<std::uint32_t>(pe_e_directory_table::architecture) || (directories[static_cast<int>(pe_e_directory_table::architecture)].m_va == 0 && directories[static_cast<int>(pe_e_directory_table::architecture)].m_size == 0), L"Architecture is reserved, must be 0.");
	WARN_M(dir_cnt < static_cast<std::uint32_t>(pe_e_directory_table::global_ptr) || directories[static_cast<int>(pe_e_directory_table::global_ptr)].m_size == 0, L"The size member of Global Ptr structure must be set to zero.");
	WARN_M(dir_cnt < static_cast<std::uint32_t>(pe_e_directory_table::reserved) || (directories[static_cast<int>(pe_e_directory_table::reserved)].m_va == 0 && directories[static_cast<int>(pe_e_directory_table::reserved)].m_size == 0), L"Reserved, must be zero.");
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)) + dir_cnt * sizeof(pe_data_directory) + coff_hdr->m_section_count * sizeof(pe_section_header)), L"File too small to contain all section headers.", false);
	pe_section_header const* const sections = reinterpret_cast<pe_section_header const*>(file_data + dosheader.m_pe_offset + (is_32 ? sizeof(pe_coff_full_32) : sizeof(pe_coff_full_64)) + dir_cnt * sizeof(pe_data_directory));
	for(std::uint16_t i = 1; i < coff_hdr->m_section_count; ++i)
	{
		WARN_M_R(sections[i].m_virtual_address > sections[i - 1].m_virtual_address, L"VAs for sections must be assigned by the linker so that they are in ascending order.", false);
	}
	for(std::uint16_t i = 0; i != coff_hdr->m_section_count; ++i)
	{
		WARN_M_R(sections[i].m_raw_ptr < 0x7fffffff, L"Section too far away.", false);
		WARN_M_R(sections[i].m_raw_size < 0x7fffffff, L"Section too big.", false);
		WARN_M_R(sections[i].m_raw_size <= 0x7fffffff - sections[i].m_raw_ptr, L"Overflow.", false);
		WARN_M_R(file_size >= static_cast<int>(sections[i].m_raw_ptr), L"File too small to contain section.", false);
		WARN_M_R(file_size >= static_cast<int>(sections[i].m_raw_ptr) + static_cast<int>(sections[i].m_raw_size), L"File too small to contain section.", false);
	}
	*header_out = &header;
	return true;
}

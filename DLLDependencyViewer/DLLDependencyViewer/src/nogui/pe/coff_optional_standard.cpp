#include "coff_optional_standard.h"

#include "mz.h"
#include "coff.h"
#include "../assert.h"


pe_e_parse_coff_optional_header_standard_32_64 pe_parse_coff_optional_header_standard_32_64(void const* const& fd, int const& file_size, pe_coff_optional_header_standard_32_64 const*& hd)
{
	char const* const file_data = static_cast<char const*>(fd);
	pe_dos_header const& dosheader = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_coff_header const& coff_hdr = *reinterpret_cast<pe_coff_header const*>(file_data + dosheader.m_pe_offset);
	WARN_M_R(coff_hdr.m_optional_header_size >= sizeof(pe_coff_optional_header_standard_32_64), L"COFF header contains too small size of coff_optional_header_standard_32_64.", pe_e_parse_coff_optional_header_standard_32_64::coff_has_wrong_optional);
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + sizeof(pe_coff_header) + sizeof(pe_coff_optional_header_standard_32_64)), L"File is too small to contain coff_optional_header_standard_32_64.", pe_e_parse_coff_optional_header_standard_32_64::file_too_small);
	hd = reinterpret_cast<pe_coff_optional_header_standard_32_64 const*>(file_data + dosheader.m_pe_offset + sizeof(pe_coff_header));
	pe_coff_optional_header_standard_32_64 const& header = *hd;
	WARN_M_R(header.m_32.m_signature == s_pe_coff_optional_sig_32 || header.m_64.m_signature == s_pe_coff_optional_sig_64, L"COFF optional signature not found.", pe_e_parse_coff_optional_header_standard_32_64::file_not_coff_optional);
	bool const is_32 = header.m_32.m_signature == s_pe_coff_optional_sig_32;
	return pe_e_parse_coff_optional_header_standard_32_64::ok;
}

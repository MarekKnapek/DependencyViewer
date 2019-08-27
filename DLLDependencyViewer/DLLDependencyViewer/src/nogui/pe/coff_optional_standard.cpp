#include "coff_optional_standard.h"

#include "mz.h"
#include "coff.h"
#include "../assert.h"


e_pe_parse_coff_optional_header_standard_32_64 pe_parse_coff_optional_header_standard_32_64(void const* const& fd, int const& file_size, coff_optional_header_standard_32_64 const*& hd)
{
	char const* const file_data = static_cast<char const*>(fd);
	dos_header const& dosheader = *reinterpret_cast<dos_header const*>(file_data + 0);
	coff_header const& coff_hdr = *reinterpret_cast<coff_header const*>(file_data + dosheader.m_pe_offset);
	WARN_M_R(coff_hdr.m_optional_header_size >= sizeof(coff_optional_header_standard_32_64), L"COFF header contains too small size of coff_optional_header_standard_32_64.", e_pe_parse_coff_optional_header_standard_32_64::coff_has_wrong_optional);
	WARN_M_R(file_size >= static_cast<int>(dosheader.m_pe_offset + sizeof(coff_header) + sizeof(coff_optional_header_standard_32_64)), L"File is too small to contain coff_optional_header_standard_32_64.", e_pe_parse_coff_optional_header_standard_32_64::file_too_small);
	hd = reinterpret_cast<coff_optional_header_standard_32_64 const*>(file_data + dosheader.m_pe_offset + sizeof(coff_header));
	coff_optional_header_standard_32_64 const& header = *hd;
	WARN_M_R(header.m_32.m_signature == s_coff_optional_sig_32 || header.m_64.m_signature == s_coff_optional_sig_64, L"COFF optional signature not found.", e_pe_parse_coff_optional_header_standard_32_64::file_not_coff_optional);
	bool const is_32 = header.m_32.m_signature == s_coff_optional_sig_32;
	return e_pe_parse_coff_optional_header_standard_32_64::ok;
}

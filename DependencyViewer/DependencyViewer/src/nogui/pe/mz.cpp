#include "mz.h"

#include "../assert_my.h"
#include "../cassert_my.h"


static constexpr std::uint16_t const s_mz_signature = 0x5a4d; // MZ


pe_e_parse_mz_header pe_parse_mz_header(std::byte const* const file_data, int const file_size, pe_dos_header const** const header_out)
{
	assert(header_out);
	WARN_M_R(file_size >= sizeof(pe_dos_header), L"File is too small to contain dos_header.", pe_e_parse_mz_header::file_too_small);
	pe_dos_header const& header = *reinterpret_cast<pe_dos_header const*>(file_data + 0);
	WARN_M_R(header.m_signature == s_mz_signature, L"MZ signature not found.", pe_e_parse_mz_header::file_not_mz);
	*header_out = &header;
	return pe_e_parse_mz_header::ok;
}

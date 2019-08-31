#include "mz.h"

#include "../assert.h"


static constexpr std::uint16_t const s_mz_signature = 0x5a4d; // MZ


pe_e_parse_mz_header pe_parse_mz_header(void const* const& fd, int const& file_size, pe_dos_header const*& hd)
{
	char const* const file_data = static_cast<char const*>(fd);
	WARN_M_R(file_size >= sizeof(pe_dos_header), L"File is too small to contain dos_header.", pe_e_parse_mz_header::file_too_small);
	hd = reinterpret_cast<pe_dos_header const*>(file_data + 0);
	pe_dos_header const& header = *hd;
	WARN_M_R(header.m_signature == s_mz_signature, L"MZ signature not found.", pe_e_parse_mz_header::file_not_mz);
	return pe_e_parse_mz_header::ok;
}

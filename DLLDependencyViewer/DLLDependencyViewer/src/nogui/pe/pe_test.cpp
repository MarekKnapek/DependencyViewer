#include "pe_test.h"

#include "mz.h"
#include "coff_full.h"
#include "../assert.h"


bool pe_test(void const* const& fd, int const& file_size)
{
	char const* const file_data = static_cast<char const*>(fd);
	dos_header const* dos_hdr;
	coff_full_32_64 const* coff_hdr;
	e_pe_parse_mz_header const dos_parsed = pe_parse_mz_header(file_data, file_size, dos_hdr);
	WARN_M_R(dos_parsed == e_pe_parse_mz_header::ok, L"Failed to pe_parse_mz_header.", false);
	bool const coff_parsed = pe_parse_coff_full_32_64(file_data, file_size, coff_hdr);
	WARN_M_R(coff_parsed, L"Failed to pe_parse_coff_full_32_64.", false);
	return true;
}

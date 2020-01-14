#include "manifest_parser.h"

#include "manifest_parser_impl.h"


bool parse_files(std::byte const* const data, int const data_len, files_t& out_files)
{
	return parse_files_impl(data, data_len, out_files);
}

#include "manifest_parser.h"

#include "assert.h"
#include "manifest_parser_impl.h"


manifest_parser::manifest_parser(memory_manager& mm) :
	m_mm(mm)
{
}

manifest_parser::~manifest_parser()
{
}

manifest_data manifest_parser::parse(std::byte const* const data, int const len)
{
	char const* const chars = reinterpret_cast<char const*>(data);
	WARN_M_R(len >= 64, L"Manifest is too short.", {});
	WARN_M_R((chars[0] == '<') || (chars[0] == '\xEF' && chars[1] == '\xBB' && chars[2] == '\xBF' && chars[3] == '<'), L"Manifest is not UTF-8 XML.", {});
	return manifest_parser_impl(*this).parse(data, len);
}

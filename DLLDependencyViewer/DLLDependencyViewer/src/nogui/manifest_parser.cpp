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

manifest_data manifest_parser::parse(char const* const& data, int const& len)
{
	WARN_M_R(len >= 64, L"Manifest is too short.", {});
	WARN_M_R((data[0] == '<') || (data[0] == '\xEF' && data[1] == '\xBB' && data[2] == '\xBF' && data[3] == '<'), L"Manifest is not UTF-8 XML.", {});
	return manifest_parser_impl(*this).parse(data, len);
}

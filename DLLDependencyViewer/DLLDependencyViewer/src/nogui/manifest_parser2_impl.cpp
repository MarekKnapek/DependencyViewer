#include "manifest_parser2_impl.h"


manifest_parser2_impl::manifest_parser2_impl(std::byte const* const data, int const data_len) :
	m_xml_parser(data, data_len)
{
}

manifest_parser2_impl::~manifest_parser2_impl()
{
}


bool manifest_parser2_impl::ok() const
{
	return m_xml_parser.ok();
}

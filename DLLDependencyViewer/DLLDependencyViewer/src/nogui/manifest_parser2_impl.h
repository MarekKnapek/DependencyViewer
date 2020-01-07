#pragma once


#include "manifest_parser2.h"
#include "xml_parser.h"

#include <cstddef>


class manifest_parser2_impl
{
public:
	manifest_parser2_impl(std::byte const* const data, int const data_len);
	~manifest_parser2_impl();
	bool ok() const;
private:
	xml_parser m_xml_parser;
};

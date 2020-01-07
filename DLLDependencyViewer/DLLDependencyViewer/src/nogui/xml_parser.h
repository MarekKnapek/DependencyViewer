#pragma once


#include "com_ptr.h"

#include <cstddef>

#include "my_windows.h"

#include <xmllite.h>


class xml_parser
{
public:
	xml_parser(std::byte const* const data, int const data_len);
	~xml_parser();
	bool ok() const;
	bool find_element(wchar_t const* const element_to_find, int const element_to_find_len, wchar_t const* const xmlns_to_find, int const xmlns_to_find_len);
private:
	com_ptr<IXmlReader> m_xml_reader;
};

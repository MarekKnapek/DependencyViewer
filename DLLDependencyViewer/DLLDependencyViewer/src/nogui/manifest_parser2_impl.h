#pragma once


#include "com_ptr.h"
#include "manifest_parser2.h"

#include <cstddef>

#include "my_windows.h"

#include <shlwapi.h>
#include <xmllite.h>


class manifest_parser2_impl
{
public:
	manifest_parser2_impl(std::byte const* const data, int const data_len);
	~manifest_parser2_impl();
	bool ok() const;
private:
	bool find_element(wchar_t const* const element_to_find, int const element_to_find_len, wchar_t const* const xmlns_to_find, int const xmlns_to_find_len);
private:
	com_ptr<IStream> m_stream;
	com_ptr<IXmlReader> m_xml_reader;
};

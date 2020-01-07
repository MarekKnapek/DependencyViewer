#pragma once


#include "com_ptr.h"

#include <cstddef>

#include "my_windows.h"

#include <xmllite.h>


typedef bool(*attribute_name_callback_t)(void* const param, wchar_t const* const attr_name, int const attr_name_len);
typedef bool(*attribute_value_callback_t)(void* const param, wchar_t const* const attr_value, int const attr_value_len);


class xml_parser
{
public:
	xml_parser(std::byte const* const data, int const data_len);
	~xml_parser();
	bool ok() const;
	bool find_element(wchar_t const* const element_to_find, int const element_to_find_len, wchar_t const* const xmlns_to_find, int const xmlns_to_find_len);
	bool for_each_attribute(void* const param, attribute_name_callback_t const name_callback, attribute_value_callback_t const value_callback);
private:
	com_ptr<IXmlReader> m_xml_reader;
};

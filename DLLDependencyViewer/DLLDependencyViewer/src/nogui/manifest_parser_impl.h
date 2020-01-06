#pragma once


#include "com_ptr.h"
#include "manifest_parser.h"

#include <cstddef>

#include "my_windows.h"

#include <shlwapi.h>
#include <xmllite.h>


class manifest_parser_impl
{
public:
	manifest_parser_impl(manifest_parser& parent);
	~manifest_parser_impl();
	manifest_data parse(std::byte const* const data, int const len);
public:
	bool parse_1();
	bool parse_2();
	bool parse_3();
	bool parse_4();
	bool parse_5();
	bool parse_6();
	bool parse_type(wchar_t const* const& value, int const& value_len);
	bool parse_name(wchar_t const* const& value, int const& value_len);
	bool parse_language(wchar_t const* const& value, int const& value_len);
	bool parse_processor_architecture(wchar_t const* const& value, int const& value_len);
	bool parse_version(wchar_t const* const& value, int const& value_len);
	bool parse_public_key_token(wchar_t const* const& value, int const& value_len);
private:
	IXmlReader& get_xml_reader() const;
	bool find_element(wchar_t const* const& element_to_find, int const& element_to_find_len, wchar_t const* const& xmlns_to_find, int const& xmlns_to_find_len);
	bool go_out_of_current_node();
private:
	manifest_parser& m_parent;
	manifest_data m_ret;
	com_ptr<IStream> m_stream;
	com_ptr<IXmlReader> m_xml_reader;
};

#include "manifest_parser.h"

#include "assert.h"
#include "manifest_parser_impl.h"

#include <iterator>


static constexpr wchar_t const s_xmllite_dll_file_name[] = L"xmllite.dll";
static constexpr char const s_create_xml_reader_function_name[] = "CreateXmlReader";

static constexpr wchar_t const s_shlwapi_dll_file_name[] = L"shlwapi.dll";
static constexpr char const s_sh_create_mem_stream_function_name[] = "SHCreateMemStream";


std::wstring name;
std::wstring version;
std::wstring arch;
std::wstring token;


void parse_manifest_1(IXmlReader* const& xml_reader);
void parse_manifest_2(IXmlReader* const& xml_reader);
void parse_manifest_3(IXmlReader* const& xml_reader);
void parse_manifest_4(IXmlReader* const& xml_reader);
void parse_manifest_5(IXmlReader* const& xml_reader);
bool find_element(IXmlReader* const& xml_reader, wchar_t const* const& xmlns_to_find, int const& xmlns_to_find_len, wchar_t const* const& element_to_find, int const& element_to_find_len);
bool assign_attribute_data(IXmlReader* const& xml_reader, std::wstring& str);
bool go_out(IXmlReader* const& xml_reader);


manifest_parser::manifest_parser(memory_manager& mm) :
	m_mm(mm),
	m_xmllite(load_library(s_xmllite_dll_file_name)),
	m_create_xml_reader(get_function_address(m_xmllite, s_create_xml_reader_function_name)),
	m_shlwapi(load_library(s_shlwapi_dll_file_name)),
	m_sh_create_mem_stream(get_function_address(m_shlwapi, s_sh_create_mem_stream_function_name))
{
	assert(m_xmllite);
	assert(m_create_xml_reader);
	assert(m_shlwapi);
	assert(m_sh_create_mem_stream);
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

#if 0

void parse_manifest_1(IXmlReader* const& xml_reader)
{
	if(find_element(xml_reader, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len, s_sxs_manifest_element_assembly, s_sxs_manifest_element_assembly_len))
	{
		parse_manifest_2(xml_reader);
	}
}

void parse_manifest_2(IXmlReader* const& xml_reader)
{
	while(find_element(xml_reader, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len, s_sxs_manifest_element_dependency, s_sxs_manifest_element_dependency_len))
	{
		parse_manifest_3(xml_reader);
	}
}

void parse_manifest_3(IXmlReader* const& xml_reader)
{
	while(find_element(xml_reader, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len, s_sxs_manifest_element_dependent_assembly, s_sxs_manifest_element_dependent_assembly_len))
	{
		parse_manifest_4(xml_reader);
	}
}

void parse_manifest_4(IXmlReader* const& xml_reader)
{
	if(find_element(xml_reader, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len, s_sxs_manifest_element_assembly_identity, s_sxs_manifest_element_assembly_identity_len))
	{
		parse_manifest_5(xml_reader);
	}
}

void parse_manifest_5(IXmlReader* const& xml_reader)
{
	UINT attr_count;
	HRESULT const got_attrib_count = xml_reader->GetAttributeCount(&attr_count);
	if(got_attrib_count != S_OK)
	{
		return;
	}
	HRESULT const moved = xml_reader->MoveToFirstAttribute();
	if(moved != S_OK)
	{
		return;
	}

	for(UINT i = 0; i != attr_count; ++i)
	{
		wchar_t const* attribute;
		UINT attribute_len;
		HRESULT const got_attribute = xml_reader->GetLocalName(&attribute, &attribute_len);
		if(got_attribute != S_OK)
		{
			return;
		}
		if(attribute_len == s_sxs_manifest_attribute_type_len && std::memcmp(attribute, s_sxs_manifest_attribute_type, (s_sxs_manifest_attribute_type_len + 1) * sizeof(wchar_t)) == 0)
		{
			wchar_t const* data;
			UINT data_len;
			HRESULT const got_data = xml_reader->GetValue(&data, &data_len);
			if(got_data != S_OK)
			{
				return;
			}
			if(data_len != s_sxs_manifest_value_type_win32_len)
			{
				return false;
			}
			if(std::memcmp(data, s_sxs_manifest_value_type_win32, (s_sxs_manifest_value_type_win32_len + 1) * sizeof(wchar_t)) != 0)
			{
				return false;
			}
		}
		else if(attribute_len == s_sxs_manifest_attribute_name_len && std::memcmp(attribute, s_sxs_manifest_attribute_name, (s_sxs_manifest_attribute_name_len + 1) * sizeof(wchar_t)) == 0)
		{
			if(!assign_attribute_data(xml_reader, name))
			{
				return false;
			}
		}
		else if(attribute_len == s_sxs_manifest_attribute_version_len && std::memcmp(attribute, s_sxs_manifest_attribute_version, (s_sxs_manifest_attribute_version_len + 1) * sizeof(wchar_t)) == 0)
		{
			if(!assign_attribute_data(xml_reader, version))
			{
				return false;
			}
		}
		else if(attribute_len == s_sxs_manifest_attribute_processor_architecture_len && std::memcmp(attribute, s_sxs_manifest_attribute_processor_architecture, (s_sxs_manifest_attribute_processor_architecture_len + 1) * sizeof(wchar_t)) == 0)
		{
			if(!assign_attribute_data(xml_reader, arch))
			{
				return false;
			}
		}
		else if(attribute_len == s_sxs_manifest_attribute_public_key_token_len && std::memcmp(attribute, s_sxs_manifest_attribute_public_key_token, (s_sxs_manifest_attribute_public_key_token_len + 1) * sizeof(wchar_t)) == 0)
		{
			if(!assign_attribute_data(xml_reader, token))
			{
				return false;
			}
		}
		HRESULT const next = xml_reader->MoveToNextAttribute();
		if(!((next == S_OK && i != attr_count - 1) || (next == S_FALSE && i == attr_count - 1)))
		{
			return false;
		}
	}
}

bool find_element(IXmlReader* const& xml_reader, wchar_t const* const& xmlns_to_find, int const& xmlns_to_find_len, wchar_t const* const& element_to_find, int const& element_to_find_len)
{
	int depth = 0;
	for(;;)
	{
		XmlNodeType node_type;
		HRESULT const read = xml_reader->Read(&node_type);
		if(read != S_OK)
		{
			continue;
		}
		if(node_type == XmlNodeType_EndElement)
		{
			if(depth == 0)
			{
				return false;
			}
			--depth;
		}
		if(node_type != XmlNodeType_Element)
		{
			continue;
		}
		++depth;
		bool const is_empty = xml_reader->IsEmptyElement() == TRUE;
		if(is_empty)
		{
			--depth;
		}

		wchar_t const* element;
		UINT element_len;
		HRESULT const got_element = xml_reader->GetLocalName(&element, &element_len);
		if(got_element != S_OK)
		{
			continue;
		}
		if(element_len != element_to_find_len)
		{
			continue;
		}
		if(std::memcmp(element, element_to_find, (element_to_find_len + 1) * sizeof(wchar_t)) != 0)
		{
			continue;
		}

		wchar_t const* xmlns;
		UINT xmlns_len;
		HRESULT const got_xmlns = xml_reader->GetNamespaceUri(&xmlns, &xmlns_len);
		if(got_xmlns != S_OK)
		{
			continue;
		}
		if(xmlns_len != xmlns_to_find_len)
		{
			continue;
		}
		if(std::memcmp(xmlns, xmlns_to_find, (xmlns_to_find_len + 1) * sizeof(wchar_t)) != 0)
		{
			continue;
		}

		if(!((depth == 1 && !is_empty) || (depth == 0 && is_empty)))
		{
			continue;
		}
		return true;
	}
	return false;
}

bool assign_attribute_data(IXmlReader* const& xml_reader, std::wstring& str)
{
	wchar_t const* data;
	UINT data_len;
	HRESULT const got_data = xml_reader->GetValue(&data, &data_len);
	if(got_data != S_OK)
	{
		return false;
	}
	str.assign(data, data + data_len);
	return true;
}

bool go_out(IXmlReader* const& xml_reader)
{
	bool const is_empty = xml_reader->IsEmptyElement() == TRUE;
	if(is_empty)
	{
		return true;
	}
	else
	{
		for(;;)
		{
			XmlNodeType node_type;
			HRESULT const read = xml_reader->Read(&node_type);
			if(read != S_OK)
			{
				return false;
			}
			if(node_type == XmlNodeType_EndElement)
			{
				return true;
			}
		}
	}
}

#endif

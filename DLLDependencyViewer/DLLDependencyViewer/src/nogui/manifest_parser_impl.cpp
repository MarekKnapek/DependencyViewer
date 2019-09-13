#include "manifest_parser_impl.h"

#include "assert.h"

#include <cassert>
#include <algorithm>

#include <shlwapi.h>


static constexpr int const s_very_big_int = 2'147'483'647;

static constexpr wchar_t const s_sxs_manifest_namespace_v1[] = L"urn:schemas-microsoft-com:asm.v1";
static constexpr int const s_sxs_manifest_namespace_v1_len = static_cast<int>(std::size(s_sxs_manifest_namespace_v1)  -1);
static constexpr wchar_t const s_sxs_manifest_element_assembly[] = L"assembly";
static constexpr int const s_sxs_manifest_element_assembly_len = static_cast<int>(std::size(s_sxs_manifest_element_assembly) - 1);
static constexpr wchar_t const s_sxs_manifest_element_dependency[] = L"dependency";
static constexpr int const s_sxs_manifest_element_dependency_len = static_cast<int>(std::size(s_sxs_manifest_element_dependency) - 1);
static constexpr wchar_t const s_sxs_manifest_element_dependent_assembly[] = L"dependentAssembly";
static constexpr int const s_sxs_manifest_element_dependent_assembly_len = static_cast<int>(std::size(s_sxs_manifest_element_dependent_assembly) - 1);
static constexpr wchar_t const s_sxs_manifest_element_assembly_identity[] = L"assemblyIdentity";
static constexpr int const s_sxs_manifest_element_assembly_identity_len = static_cast<int>(std::size(s_sxs_manifest_element_assembly_identity) - 1);
static constexpr wchar_t const s_sxs_manifest_attribute_type[] = L"type";
static constexpr int const s_sxs_manifest_attribute_type_len = static_cast<int>(std::size(s_sxs_manifest_attribute_type) - 1);
static constexpr wchar_t const s_sxs_manifest_attribute_name[] = L"name";
static constexpr int const s_sxs_manifest_attribute_name_len = static_cast<int>(std::size(s_sxs_manifest_attribute_name) - 1);
static constexpr wchar_t const s_sxs_manifest_attribute_language[] = L"language";
static constexpr int const s_sxs_manifest_attribute_language_len = static_cast<int>(std::size(s_sxs_manifest_attribute_language) - 1);
static constexpr wchar_t const s_sxs_manifest_attribute_processor_architecture[] = L"processorArchitecture";
static constexpr int const s_sxs_manifest_attribute_processor_architecture_len = static_cast<int>(std::size(s_sxs_manifest_attribute_processor_architecture) - 1);
static constexpr wchar_t const s_sxs_manifest_attribute_version[] = L"version";
static constexpr int const s_sxs_manifest_attribute_version_len = static_cast<int>(std::size(s_sxs_manifest_attribute_version) - 1);
static constexpr wchar_t const s_sxs_manifest_attribute_public_key_token[] = L"publicKeyToken";
static constexpr int const s_sxs_manifest_attribute_public_key_token_len = static_cast<int>(std::size(s_sxs_manifest_attribute_public_key_token) - 1);
static constexpr wchar_t const s_sxs_manifest_value_type_win32[] = L"win32";
static constexpr int const s_sxs_manifest_value_type_win32_len = static_cast<int>(std::size(s_sxs_manifest_value_type_win32) - 1);
static constexpr wchar_t const s_sxs_manifest_value_arch_x86[] = L"x86";
static constexpr int const s_sxs_manifest_value_arch_x86_len = static_cast<int>(std::size(s_sxs_manifest_value_arch_x86) - 1);
static constexpr wchar_t const s_sxs_manifest_value_arch_ia64[] = L"ia64";
static constexpr int const s_sxs_manifest_value_arch_ia64_len = static_cast<int>(std::size(s_sxs_manifest_value_arch_ia64) - 1);
static constexpr wchar_t const s_sxs_manifest_value_arch_amd64[] = L"amd64";
static constexpr int const s_sxs_manifest_value_arch_amd64_len = static_cast<int>(std::size(s_sxs_manifest_value_arch_amd64) - 1);
static constexpr wchar_t const s_sxs_manifest_value_arch_star[] = L"*";
static constexpr int const s_sxs_manifest_value_arch_star_len = static_cast<int>(std::size(s_sxs_manifest_value_arch_star) - 1);


static constexpr int const s_assembly_identity_attribute_lens[] =
{
	s_sxs_manifest_attribute_type_len,
	s_sxs_manifest_attribute_name_len,
	s_sxs_manifest_attribute_language_len,
	s_sxs_manifest_attribute_processor_architecture_len,
	s_sxs_manifest_attribute_version_len,
	s_sxs_manifest_attribute_public_key_token_len
};
static constexpr wchar_t const* const s_assembly_identity_attribute_names[] =
{
	s_sxs_manifest_attribute_type,
	s_sxs_manifest_attribute_name,
	s_sxs_manifest_attribute_language,
	s_sxs_manifest_attribute_processor_architecture,
	s_sxs_manifest_attribute_version,
	s_sxs_manifest_attribute_public_key_token
};
using my_mem_fn = bool(manifest_parser_impl::*)(wchar_t const* const& value, int const& value_len);
static constexpr my_mem_fn const s_assembly_identity_attribute_funcs[] =
{
	&manifest_parser_impl::parse_type,
	&manifest_parser_impl::parse_name,
	&manifest_parser_impl::parse_language,
	&manifest_parser_impl::parse_processor_architecture,
	&manifest_parser_impl::parse_version,
	&manifest_parser_impl::parse_public_key_token,
};
static_assert(std::size(s_assembly_identity_attribute_lens) == std::size(s_assembly_identity_attribute_names), "");
static_assert(std::size(s_assembly_identity_attribute_lens) == std::size(s_assembly_identity_attribute_funcs), "");


static constexpr int const s_sxs_archs_lens[] =
{
	s_sxs_manifest_value_arch_x86_len,
	s_sxs_manifest_value_arch_ia64_len,
	s_sxs_manifest_value_arch_amd64_len,
	s_sxs_manifest_value_arch_star_len
};
static constexpr wchar_t const* const s_sxs_archs_names[] =
{
	s_sxs_manifest_value_arch_x86,
	s_sxs_manifest_value_arch_ia64,
	s_sxs_manifest_value_arch_amd64,
	s_sxs_manifest_value_arch_star
};
static_assert(std::size(s_sxs_archs_lens) == std::size(s_sxs_archs_names), "");


manifest_parser_impl::manifest_parser_impl(manifest_parser& parent) :
	m_parent(parent),
	m_ret(),
	m_stream(),
	m_xml_reader()
{
}

manifest_parser_impl::~manifest_parser_impl()
{
}

manifest_data manifest_parser_impl::parse(char const* const& data, int const& len)
{
	IStream* const stream = SHCreateMemStream(reinterpret_cast<BYTE const*>(data), static_cast<UINT>(len));
	WARN_M_R(stream, L"Failed to SHCreateMemStream.", {});
	m_stream.reset(stream);

	IXmlReader* xml_reader;
	HRESULT const xml_reader_created = CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>(&xml_reader), nullptr);
	WARN_M_R(xml_reader_created == S_OK && xml_reader, L"Failed to CreateXmlReader.", {});
	m_xml_reader.reset(xml_reader);

	HRESULT const dtd_set = xml_reader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);
	WARN_M_R(dtd_set == S_OK, L"Failed to IXmlReader::SetProperty(DtdProcessing, Prohibit).", {});

	HRESULT const input_set = xml_reader->SetInput(m_stream.get());
	WARN_M_R(input_set == S_OK, L"Failed to IXmlReader::SetInput.", {});

	bool const parsed_1 = parse_1();
	WARN_M_R(parsed_1, L"Failed to parse_1.", {});

	return m_ret;
}

bool manifest_parser_impl::parse_1()
{
	bool const found = find_element(s_sxs_manifest_element_assembly, s_sxs_manifest_element_assembly_len, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len);
	WARN_M_R(found, L"Failed to find assembly element.", false);
	bool const parsed_2 = parse_2();
	WARN_M_R(parsed_2, L"Failed to parse_2.", false);
	return true;
}

bool manifest_parser_impl::parse_2()
{
	while(find_element(s_sxs_manifest_element_dependency, s_sxs_manifest_element_dependency_len, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len))
	{
		bool const parsed_3 = parse_3();
		WARN_M_R(parsed_3, L"Failed to parse_3.", false);
	}
	return true;
}

bool manifest_parser_impl::parse_3()
{
	bool const found = find_element(s_sxs_manifest_element_dependent_assembly, s_sxs_manifest_element_dependent_assembly_len, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len);
	WARN_M_R(found, L"Failed to find at least one dependentAssembly element.", false);
	bool const parsed_4 = parse_4();
	WARN_M_R(parsed_4, L"Failed to parse_4.", false);
	bool const closed = go_out_of_current_node();
	WARN_M_R(closed, L"Failed to go_out_of_current_node.", false);
	while(find_element(s_sxs_manifest_element_dependent_assembly, s_sxs_manifest_element_dependent_assembly_len, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len))
	{
		bool const parsed_4 = parse_4();
		WARN_M_R(parsed_4, L"Failed to parse_4.", false);
		bool const closed = go_out_of_current_node();
		WARN_M_R(closed, L"Failed to go_out_of_current_node.", false);
	}
	return true;
}

bool manifest_parser_impl::parse_4()
{
	bool const found = find_element(s_sxs_manifest_element_assembly_identity, s_sxs_manifest_element_assembly_identity_len, s_sxs_manifest_namespace_v1, s_sxs_manifest_namespace_v1_len);
	WARN_M_R(found, L"Failed to find dependent assemblyIdentity element.", false);
	m_ret.m_dependencies.push_back({});
	bool const parsed_5 = parse_5();
	WARN_M_R(parsed_5, L"Failed to parse_5.", false);
	bool const closed = go_out_of_current_node();
	WARN_M_R(closed, L"Failed to go_out_of_current_node.", false);
	return true;
}

bool manifest_parser_impl::parse_5()
{
	IXmlReader& xml_reader = get_xml_reader();
	UINT attr_count;
	HRESULT const got_attrib_count = xml_reader.GetAttributeCount(&attr_count);
	WARN_M_R(got_attrib_count == S_OK, L"Failed to IXmlReader::GetAttributeCount.", false);
	HRESULT const moved = xml_reader.MoveToFirstAttribute();
	WARN_M_R(moved == S_OK, L"Failed to IXmlReader::MoveToFirstAttribute.", false);
	for(UINT i = 0; i != attr_count; ++i)
	{
		bool const parsed_6 = parse_6();
		WARN_M_R(parsed_6, L"Failed to parse_6.", false);
		HRESULT const next = xml_reader.MoveToNextAttribute();
		WARN_M_R((next == S_OK && i != attr_count - 1) || (next == S_FALSE && i == attr_count - 1), L"Failed to IXmlReader::MoveToNextAttribute.", false);
	}
	HRESULT const moved_back = xml_reader.MoveToElement();
	WARN_M_R(moved_back == S_OK, L"Failed to IXmlReader::MoveToElement.", false);
	return true;
}

bool manifest_parser_impl::parse_6()
{
	IXmlReader& xml_reader = get_xml_reader();
	wchar_t const* attribute;
	UINT attribute_len;
	HRESULT const got_attribute = xml_reader.GetLocalName(&attribute, &attribute_len);
	WARN_M_R(got_attribute == S_OK && attribute != nullptr, L"Failed to IXmlReader::GetLocalName.", false);
	bool found = false;
	for(int i = 0; i != static_cast<int>(std::size(s_assembly_identity_attribute_lens)); ++i)
	{
		if(attribute_len == s_assembly_identity_attribute_lens[i] && std::memcmp(attribute, s_assembly_identity_attribute_names[i], (s_assembly_identity_attribute_lens[i] + 1) * sizeof(wchar_t)) == 0)
		{
			found = true;
			wchar_t const* value;
			UINT value_len;
			HRESULT const got_value = xml_reader.GetValue(&value, &value_len);
			WARN_M_R(got_value == S_OK, L"Failed to IXmlReader::GetValue.", false);
			WARN_M_R(value_len < s_very_big_int, L"Attribute is too big.", false);
			bool const parsed = (this->*s_assembly_identity_attribute_funcs[i])(value, static_cast<int>(value_len));
			WARN_M_R(parsed, L"Failed to parse attribute.", false);
			break;
		}
	}
	WARN_M_R(found, L"Failed to parse unknown attribute.", false);
	return true;
}

bool manifest_parser_impl::parse_type(wchar_t const* const& value, int const& value_len)
{
	WARN_M_R(value_len == s_sxs_manifest_value_type_win32_len, L"Type must be win32.", false);
	WARN_M_R(std::memcmp(value, s_sxs_manifest_value_type_win32, (s_sxs_manifest_value_type_win32_len + 1) * sizeof(wchar_t)) == 0, L"Type must be win32.", false);
	return true;
}

bool manifest_parser_impl::parse_name(wchar_t const* const& value, int const& value_len)
{
	m_ret.m_dependencies.back().m_name = m_parent.m_mm.m_wstrs.add_string(value, value_len, m_parent.m_mm.m_alc);
	return true;
}

bool manifest_parser_impl::parse_language(wchar_t const* const& value, int const& value_len)
{
	m_ret.m_dependencies.back().m_language = m_parent.m_mm.m_wstrs.add_string(value, value_len, m_parent.m_mm.m_alc);
	return true;
}

bool manifest_parser_impl::parse_processor_architecture(wchar_t const* const& value, int const& value_len)
{
	WARN_M_R(value_len <= 6, L"Failed to parse unknown processor architecture.", false);
	wchar_t buff[6];
	std::transform(value, value + value_len + 1, buff, [](wchar_t const& e) -> wchar_t { if(e >= L'A' && e <= L'Z'){ return L'a' + (e - L'A'); } else { return e; } });

	bool found = false;
	for(int i = 0; i != static_cast<int>(std::size(s_sxs_archs_lens)); ++i)
	{
		if(value_len == s_sxs_archs_lens[i] && std::memcmp(buff, s_sxs_archs_names[i], (s_sxs_archs_lens[i] + 1) * sizeof(wchar_t)) == 0)
		{
			found = true;
			m_ret.m_dependencies.back().m_architecture = static_cast<manifest_dependency_architecture>(i);
			break;
		}
	}
	WARN_M_R(found, L"Failed to parse unknown processor architecture.", false);

	return true;
}

bool manifest_parser_impl::parse_version(wchar_t const* const& value, int const& value_len)
{
	return true;
}

bool manifest_parser_impl::parse_public_key_token(wchar_t const* const& value, int const& value_len)
{
	return true;
}

IXmlReader& manifest_parser_impl::get_xml_reader() const
{
	return *static_cast<IXmlReader*>(m_xml_reader.get());
}

bool manifest_parser_impl::find_element(wchar_t const* const& element_to_find, int const& element_to_find_len, wchar_t const* const& xmlns_to_find, int const& xmlns_to_find_len)
{
	IXmlReader& xml_reader = get_xml_reader();
	int depth = 0;
	for(;;)
	{
		XmlNodeType node_type;
		HRESULT const read = xml_reader.Read(&node_type);
		WARN_M_R(read == S_OK, L"Failed to IXmlReader::Read.", false);
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
		bool const is_empty = xml_reader.IsEmptyElement() == TRUE;
		if(is_empty)
		{
			--depth;
		}

		wchar_t const* element;
		UINT element_len;
		HRESULT const got_element = xml_reader.GetLocalName(&element, &element_len);
		WARN_M_R(got_element == S_OK, L"Failed to IXmlReader::GetLocalName.", false);
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
		HRESULT const got_xmlns = xml_reader.GetNamespaceUri(&xmlns, &xmlns_len);
		WARN_M_R(got_xmlns == S_OK && xmlns != nullptr, L"Failed to IXmlReader::GetNamespaceUri.", false);
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
	assert(false);
	return false;
}

bool manifest_parser_impl::go_out_of_current_node()
{
	IXmlReader& xml_reader = get_xml_reader();
	if(xml_reader.IsEmptyElement() == TRUE)
	{
		return true;
	}
	int depth = 0;
	for(;;)
	{
		XmlNodeType node_type;
		HRESULT const read = xml_reader.Read(&node_type);
		WARN_M_R(read == S_OK, L"Failed to IXmlReader::Read.", false);
		if(node_type == XmlNodeType_EndElement)
		{
			if(depth == 0)
			{
				return true;
			}
			--depth;
		}
		else if(node_type == XmlNodeType_Element)
		{
			bool const is_empty = xml_reader.IsEmptyElement() == TRUE;
			if(!is_empty)
			{
				++depth;
			}
		}
	}
	return false;
}

#include "manifest_parser_impl.h"

#include "assert.h"

#include <cassert>
#include <cstring>
#include <iterator>


static constexpr wchar_t const s_manifest_namespace_v1[] = L"urn:schemas-microsoft-com:asm.v1";

static constexpr wchar_t const s_manifest_element_assembly[] = L"assembly";
static constexpr wchar_t const s_manifest_element_file[] = L"file";

static constexpr wchar_t const s_manifest_attribute_name[] = L"name";

static constexpr int const s_manifest_namespace_v1_len = static_cast<int>(std::size(s_manifest_namespace_v1)) - 1;

static constexpr int const s_manifest_element_assembly_len = static_cast<int>(std::size(s_manifest_element_assembly)) - 1;
static constexpr int const s_manifest_element_file_len = static_cast<int>(std::size(s_manifest_element_file)) - 1;

static constexpr int const s_manifest_attribute_name_len = static_cast<int>(std::size(s_manifest_attribute_name)) - 1;


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

bool manifest_parser2_impl::parse_files(files_t& out_files)
{
	bool const assembly_found = find_assembly();
	WARN_M_R(assembly_found, L"Failed to find_assembly.", false);
	while(find_file())
	{
		bool const parsed_1 = parse_files_1(out_files);
		WARN_M_R(parsed_1, L"Failed to parse_files_1.", false);
	}
	return true;
}

bool manifest_parser2_impl::parse_files_1(files_t& out_files)
{
	static constexpr auto const fn_attr_name_ = []([[maybe_unused]] void* const param, wchar_t const* const attr_name, int const attr_name_len) -> bool
	{
		return
			attr_name_len == s_manifest_attribute_name_len &&
			std::memcmp(attr_name, s_manifest_attribute_name, (s_manifest_attribute_name_len + 1) * sizeof(wchar_t)) == 0;
	};

	static constexpr auto const fn_attr_value_ = [](void* const param, wchar_t const* const attr_name, int const attr_name_len) -> bool
	{
		assert(param);
		files_t& out_files = *static_cast<files_t*>(param);
		out_files.emplace_back(attr_name, attr_name_len);
		return true;
	};

	attribute_name_callback_t const fn_attr_name = fn_attr_name_;
	attribute_value_callback_t const fn_attr_value = fn_attr_value_;
	bool const attributes_processed = m_xml_parser.for_each_attribute(&out_files, fn_attr_name, fn_attr_value);
	WARN_M_R(attributes_processed, L"Failed to xml_parser::for_each_attribute.", false);
	return true;
}

bool manifest_parser2_impl::find_assembly()
{
	return m_xml_parser.find_element(s_manifest_element_assembly, s_manifest_element_assembly_len, s_manifest_namespace_v1, s_manifest_namespace_v1_len);
}

bool manifest_parser2_impl::find_file()
{
	return m_xml_parser.find_element(s_manifest_element_file, s_manifest_element_file_len, s_manifest_namespace_v1, s_manifest_namespace_v1_len);
}


bool parse_files_impl(std::byte const* const data, int const data_len, files_t& out_files)
{
	manifest_parser2_impl mp{data, data_len};
	WARN_M_R(mp.ok(), L"Failed to create manifest_parser2_impl.", false);
	return mp.parse_files(out_files);
}
